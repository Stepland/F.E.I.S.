// FindTempo_standalone.cpp
// Originally by Fietsemaker (Bram van de Wetering) and adapted to a standalone format by Nathan Stephenson


#include "FindTempo_standalone.hpp"

namespace vortex {

using namespace std;

void FindOnsets(float* samples, int samplerate, int numFrames, int numThreads, Vector<Onset>* onsets) {
    int window = 1024; // window size
    int hop_size = window / 4;
	unsigned int read = 0;  // bytes read each loop
    unsigned int total = 0;  // total bytes read
    
    fvec_t* in = new_fvec(hop_size); // input audio buffer
    fvec_t* out = new_fvec(2); // output position
	
    aubio_onset_t* o = new_aubio_onset("complex", window, hop_size, samplerate);
    do {
		for (int i = 0; i < hop_size; i++) {
			// if (total + i >= numFrames) PrintOut("exceeding numFrames")
			in->data[i] = (total+i < numFrames) ? samples[total+i] : 0;
		}

        aubio_onset_do(o, in, out);

        // if (out->data[0] > 0) // if there is an onset
		if (out->data[0] > 0 && aubio_onset_get_last(o) >= 0) {
			// printf("%d\n", aubio_onset_get_last(o));
			onsets->append(Onset(aubio_onset_get_last(o), 1.0));
		}
		
		total += hop_size;
    } while (total < numFrames);
    
    // cleanup
    // del_aubio_source(source);
    del_aubio_onset(o);
    del_fvec(in);
    del_fvec(out);
}

namespace find_tempo_cpp {

static const real_t MinimumBPM = 89.0;
static const real_t MaximumBPM = 205.0;
static const int IntervalDelta = 10;
static const int IntervalDownsample = 3;
static const int MaxThreads = 8;

// ================================================================================================
// Helper structs.

struct TempoResult
{
	TempoResult() : fitness(0.0f), bpm(0.0f), offset(0.0f) {}

	float fitness;
    float bpm;
    float offset;

	TempoResult(float a, float b, float c) {
		bpm = a; offset = b; fitness = c;
	}
};

typedef Vector<TempoResult> TempoResults;
typedef unsigned char uchar;

struct TempoSort {
	bool operator()(const TempoResult& a, const TempoResult& b) {
		return a.fitness > b.fitness;
	}
};

struct SerializedTempo
{
	SerializedTempo() : onsets(nullptr) {}

	float* samples;
	int samplerate;
	int numFrames;
	int numThreads;
	Vector<Onset>* onsets;
	uchar* terminate;
	TempoResults result;
};

struct GapData
{
	GapData(int numThreads, int maxInterval, int downsample, int numOnsets, const Onset* onsets);
	~GapData();

	const Onset* onsets;
	int* wrappedPos;
	real_t* wrappedOnsets;
	real_t* window;
	int bufferSize, numOnsets, windowSize, downsample;
};

struct IntervalTester
{
	IntervalTester(int samplerate, int numOnsets, const Onset* onsets);
	~IntervalTester();

	int minInterval;
	int maxInterval;
	int numIntervals;
	int samplerate;
	int gapWindowSize;
	int numOnsets;
	const Onset* onsets;
	real_t* fitness;
	real_t coefs[4];
};

// ================================================================================================
// Audio processing

// Creates weights for a hamming window of length n.
static void CreateHammingWindow(real_t* out, int n)
{
	const real_t t = 6.2831853071795864 / (real_t)(n - 1);
	for (int i = 0; i < n; ++i) out[i] = 0.54 - 0.46 * cos((real_t)i * t);
}

// Normalizes the given fitness value based the given 3rd order poly coefficients and interval.
static void NormalizeFitness(real_t& fitness, const real_t* coefs, real_t interval)
{
	real_t x = interval, x2 = x * x, x3 = x2 * x;
	fitness -= coefs[0] + coefs[1] * x + coefs[2] * x2 + coefs[3] * x3;
}

// ================================================================================================
// Gap confidence evaluation

GapData::GapData(int numThreads, int bufferSize, int downsample, int numOnsets, const Onset* onsets)
	: numOnsets(numOnsets)
	, onsets(onsets)
	, downsample(downsample)
	, windowSize(2048 >> downsample)
	, bufferSize(bufferSize)
{
	window = AlignedMalloc(real_t, windowSize);
	wrappedPos = AlignedMalloc(int, numOnsets * numThreads);
	wrappedOnsets = AlignedMalloc(real_t, bufferSize * numThreads);
	CreateHammingWindow(window, windowSize);
}

GapData::~GapData()
{
	AlignedFree(window);
	AlignedFree(wrappedPos);
	AlignedFree(wrappedOnsets);
}

// Returns the confidence value that indicates how many onsets are close to the given gap position.
static real_t GapConfidence(const GapData& gapdata, int threadId, int gapPos, int interval)
{
	int numOnsets = gapdata.numOnsets;
	int windowSize = gapdata.windowSize;
	int halfWindowSize = windowSize / 2;
	const real_t* window = gapdata.window;
	const real_t* wrappedOnsets = gapdata.wrappedOnsets + gapdata.bufferSize * threadId;
	real_t area = 0.0;

	int beginOnset = gapPos - halfWindowSize;
	int endOnset = gapPos + halfWindowSize;

	if (beginOnset < 0)
	{
		int wrappedBegin = beginOnset + interval;
		for (int i = wrappedBegin; i < interval; ++i)
		{
			int windowIndex = i - wrappedBegin;
			area += wrappedOnsets[i] * window[windowIndex];
		}
		beginOnset = 0;
	}
	if (endOnset > interval)
	{
		int wrappedEnd = endOnset - interval;
		int indexOffset = windowSize - wrappedEnd;
		for (int i = 0; i < wrappedEnd; ++i)
		{
			int windowIndex = i + indexOffset;
			area += wrappedOnsets[i] * window[windowIndex];
		}
		endOnset = interval;
	}
	for (int i = beginOnset; i < endOnset; ++i)
	{
		int windowIndex = i - beginOnset;
		area += wrappedOnsets[i] * window[windowIndex];
	}

	return area;
}

// Returns the confidence of the best gap value for the given interval.
static real_t GetConfidenceForInterval(const GapData& gapdata, int threadId, int interval)
{
	int downsample = gapdata.downsample;
	int numOnsets = gapdata.numOnsets;
	const Onset* onsets = gapdata.onsets;

	int* wrappedPos = gapdata.wrappedPos + gapdata.numOnsets * threadId;
	real_t* wrappedOnsets = gapdata.wrappedOnsets + gapdata.bufferSize * threadId;
	memset(wrappedOnsets, 0, sizeof(real_t) * gapdata.bufferSize);

	// Make a histogram of onset strengths for every position in the interval.
	int reducedInterval = interval >> downsample;
	for (int i = 0; i < numOnsets; ++i)
	{
		int pos = (onsets[i].pos % interval) >> downsample;
		wrappedPos[i] = pos;
		// printf("%f\n", onsets[i].strength);
		wrappedOnsets[pos] += onsets[i].strength;
	}

	// Record the amount of support for each gap value.
	real_t highestConfidence = 0.0;
	for (int i = 0; i < numOnsets; ++i)
	{
		int pos = wrappedPos[i];
		real_t confidence = GapConfidence(gapdata, threadId, pos, reducedInterval);
		int offbeatPos = (pos + reducedInterval / 2) % reducedInterval;
		confidence += GapConfidence(gapdata, threadId, offbeatPos, reducedInterval) * 0.5;

		if (confidence > highestConfidence)
		{
			highestConfidence = confidence;
		}
	}
	// printf("%f\n", highestConfidence);
	return highestConfidence;
}

// Returns the confidence of the best gap value for the given BPM value.
static real_t GetConfidenceForBPM(const GapData& gapdata, int threadId, IntervalTester& test, real_t bpm)
{
	assert(bpm > 0);
	int numOnsets = gapdata.numOnsets;
	const Onset* onsets = gapdata.onsets;

	int* wrappedPos = gapdata.wrappedPos + gapdata.numOnsets * threadId;
	real_t* wrappedOnsets = gapdata.wrappedOnsets + gapdata.bufferSize * threadId;
	// printf("%d\n\n", gapdata.bufferSize);
	memset(wrappedOnsets, 0, sizeof(real_t) * gapdata.bufferSize);

	// Make a histogram of i strengths for every position in the interval.
	real_t intervalf = test.samplerate * 60.0 / bpm;
	int interval = (int)(intervalf + 0.5);
	for (int i = 0; i < numOnsets; ++i)
	{
		// printf("%d out of %d\n", i+1, numOnsets);
		// PrintOut(string(onsets[i]));
		int pos = (int)fmod((real_t)onsets[i].pos, intervalf);
		wrappedPos[i] = pos;
		// printf("%d\n", pos);
		wrappedOnsets[pos] += onsets[i].strength;  // error here @ 252837
	}

	// Record the amount of support for each gap value.
	real_t highestConfidence = 0.0;
	for (int i = 0; i < numOnsets; ++i)
	{
		int pos = wrappedPos[i];
		real_t confidence = GapConfidence(gapdata, threadId, pos, interval);
		int offbeatPos = (pos + interval / 2) % interval;
		confidence += GapConfidence(gapdata, threadId, offbeatPos, interval) * 0.5;

		if (confidence > highestConfidence)
		{
			highestConfidence = confidence;
		}
	}

	// Normalize the confidence value.
	NormalizeFitness(highestConfidence, test.coefs, intervalf);

	return highestConfidence;
}

// ================================================================================================
// Interval testing

IntervalTester::IntervalTester(int samplerate, int numOnsets, const Onset* onsets)
	: samplerate(samplerate)
	, numOnsets(numOnsets)
	, onsets(onsets)
{
	minInterval = (int)(samplerate * 60.0 / MaximumBPM + 0.5);
	maxInterval = (int)(samplerate * 60.0 / MinimumBPM + 0.5);
	numIntervals = maxInterval - minInterval;

	fitness = AlignedMalloc(real_t, numIntervals);
}

IntervalTester::~IntervalTester()
{
	AlignedFree(fitness);
}

static real_t IntervalToBPM(const IntervalTester& test, int i)
{
	return (test.samplerate * 60.0) / (i + test.minInterval);
}

static void FillCoarseIntervals(IntervalTester& test, GapData& gapdata, int numThreads)
{
	int numCoarseIntervals = (test.numIntervals + IntervalDelta - 1) / IntervalDelta;
	if (numThreads > 1)
	{
		struct IntervalThreads : public ParallelThreads
		{
			IntervalTester* test;
			GapData* gapdata;
			void execute(int i, int t) override
			{
				int index = i * IntervalDelta;
				// printf("%d\n", index);
				int interval = test->minInterval + index;
				test->fitness[index] = max(0.001, GetConfidenceForInterval(*gapdata, t, interval));
			}
		};
		IntervalThreads threads;
		threads.test = &test;
		threads.gapdata = &gapdata;
		threads.run(numCoarseIntervals, numThreads);
	}
	else
	{
		for (int i = 0; i < numCoarseIntervals; ++i)
		{
			int index = i * IntervalDelta;
			int interval = test.minInterval + index;
			test.fitness[index] = max(0.001, GetConfidenceForInterval(gapdata, 0, interval));
		}
	}
}

static vec2i FillIntervalRange(IntervalTester& test, GapData& gapdata, int begin, int end)
{
	begin = max(begin, 0);
	end = min(end, test.numIntervals);
	real_t* fit = test.fitness + begin;
	for (int i = begin, interval = test.minInterval + begin; i < end; ++i, ++interval, ++fit)
	{
		if (*fit == 0)
		{
			*fit = GetConfidenceForInterval(gapdata, 0, interval);
			NormalizeFitness(*fit, test.coefs, (real_t)interval);
			*fit = max(*fit, 0.1);
		}
	}
	return {begin, end};
}

static int FindBestInterval(const real_t* fitness, int begin, int end)
{
	int bestInterval = 0;
	real_t highestFitness = 0.0;
	for (int i = begin; i < end; ++i)
	{
		if (fitness[i] > highestFitness)
		{
			highestFitness = fitness[i];
			bestInterval = i;
		}
	}
	return bestInterval;
}

// ================================================================================================
// BPM testing

// Removes BPM values that are near-duplicates or multiples of a better BPM value.
static void RemoveDuplicates(TempoResults& tempo)
{
	for (int i = 0; i < tempo.size(); ++i)
	{
		real_t bpm = tempo[i].bpm, doubled = bpm * 2.0, halved = bpm * 0.5;
		for (int j = tempo.size() - 1; j > i; --j)
		{
			real_t v = tempo[j].bpm;
			if (min(min(abs(v - bpm), abs(v - doubled)), abs(v - halved)) < 0.1)
			{
				// printf("Erasing %f\n", tempo[j].bpm);
				tempo.erase(j);
			}
		}
	}
}

// Rounds BPM values that are close to integer values.
static void RoundBPMValues(IntervalTester& test, GapData& gapdata, TempoResults& tempo)
{
	for (auto& t : tempo)
	{
		real_t roundBPM = round(t.bpm);
		real_t diff = abs(t.bpm - roundBPM);
		if (diff < 0.01)
		{
			t.bpm = roundBPM;
		}
		else if (diff < 0.05)
		{
			real_t old = GetConfidenceForBPM(gapdata, 0, test, t.bpm);
			real_t cur = GetConfidenceForBPM(gapdata, 0, test, roundBPM);
			if (cur > old * 0.99) t.bpm = roundBPM;
		}
	}
}

// Finds likely BPM candidates based on the given note onset values.
static void CalculateBPM(SerializedTempo* data, Onset* onsets, int numOnsets)
{
	auto& tempo = data->result;

	// In order to determine the BPM, we need at least two onsets.
	if (numOnsets < 2)
	{
		PrintErr("Onset count is less than 2, skipping BPM calculation...");
		tempo.append(100.0, 0.0, 1.0);
		return;
	}

	IntervalTester test(data->samplerate, numOnsets, onsets);
	GapData* gapdata = new GapData(data->numThreads, test.maxInterval, IntervalDownsample, numOnsets, onsets);

	// Loop through every 10th possible BPM, later we will fill in those that look interesting.
	memset(test.fitness, 0, test.numIntervals * sizeof(real_t));
	FillCoarseIntervals(test, *gapdata, data->numThreads);
	int numCoarseIntervals = (test.numIntervals + IntervalDelta - 1) / IntervalDelta;
	MarkProgress(2, "Fill coarse intervals");

	// Determine the polynomial coefficients to approximate the fitness curve and normalize the current fitness values.
	mathalgo::polyfit(3, test.coefs, test.fitness, numCoarseIntervals, test.minInterval);
	// polyfit(3, test.coefs, test.fitness, numCoarseIntervals, test.minInterval, IntervalDelta);
	real_t maxFitness = 0.001;
	for (int i = 0; i < test.numIntervals; i += IntervalDelta)
	{
		NormalizeFitness(test.fitness[i], test.coefs, (real_t)(test.minInterval + i));
		maxFitness = max(maxFitness, test.fitness[i]);
	}

	// Refine the intervals around the best intervals.
	real_t fitnessThreshold = maxFitness * 0.4;
	for (int i = 0; i < test.numIntervals; i += IntervalDelta)
	{
		if (test.fitness[i] > fitnessThreshold)
		{
			vec2i range = FillIntervalRange(test, *gapdata, i - IntervalDelta, i + IntervalDelta);
			int best = FindBestInterval(test.fitness, range.x, range.y);
			tempo.append(IntervalToBPM(test, best), 0.0, test.fitness[best]);
		}
	}
	MarkProgress(3, "Refine intervals");

	// At this point we stop the downsampling and upgrade to a more precise gap window.
	delete gapdata;
	gapdata = new GapData(data->numThreads, test.maxInterval, 0, numOnsets, onsets);

	// Round BPM values to integers when possible, and remove weaker duplicates.
	std::stable_sort(tempo.begin(), tempo.end(), TempoSort());
	RemoveDuplicates(tempo);
	RoundBPMValues(test, *gapdata, tempo);

	// If the fitness of the first and second option is very close, we ask for a second opinion.
	if (tempo.size() >= 2 && tempo[0].fitness / tempo[1].fitness < 1.05)
	{
		for (auto& t : tempo)
			t.fitness = GetConfidenceForBPM(*gapdata, 0, test, t.bpm);
		std::stable_sort(tempo.begin(), tempo.end(), TempoSort());
	}

	// In all 300 test cases the correct BPM value was part of the top 3 choices,
	// so it seems reasonable to discard anything below the top 3 as irrelevant.
	if (tempo.size() > 3) tempo.resize(3);

	// Cleanup.
	delete gapdata;
}

// ================================================================================================
// Offset testing

static void ComputeSlopes(const float* samples, real_t* out, int numFrames, int samplerate)
{
	memset(out, 0, sizeof(real_t) * numFrames);

	int wh = samplerate / 20;
	if (numFrames < wh * 2) return;

	// Initial sums of the left/right side of the window.
	real_t sumL = 0, sumR = 0;
	for (int i = 0, j = wh; i < wh; ++i, ++j)
	{
		sumL += abs(samples[i]);
		sumR += abs(samples[j]);
	}

	// Slide window over the samples.
	real_t scalar = 1.0 / (real_t)wh;
	for (int i = wh, end = numFrames - wh; i < end; ++i)
	{
		// Determine slope value.
		out[i] = max(0.0, (real_t)(sumR - sumL) * scalar);

		// Move window.
		real_t cur = abs(samples[i]);
		sumL -= abs(samples[i - wh]);
		sumL += cur;
		sumR -= cur;
		sumR += abs(samples[i + wh]);
	}
}

// Returns the most promising offset for the given BPM value.
static real_t GetBaseOffsetValue(const GapData& gapdata, int samplerate, real_t bpm)
{
	int numOnsets = gapdata.numOnsets;
	const Onset* onsets = gapdata.onsets;

	int* wrappedPos = gapdata.wrappedPos;
	real_t* wrappedOnsets = gapdata.wrappedOnsets;
	memset(wrappedOnsets, 0, sizeof(real_t) * gapdata.bufferSize);

	// Make a histogram of onset strengths for every position in the interval.
	real_t intervalf = samplerate * 60.0 / bpm;
	int interval = (int)(intervalf + 0.5);
	memset(wrappedOnsets, 0, sizeof(real_t) * interval);
	for (int i = 0; i < numOnsets; ++i)
	{
		int pos = (int)fmod((real_t)onsets[i].pos, intervalf);
		wrappedPos[i] = pos;
		wrappedOnsets[pos] += 1.0;
	}

	// Record the amount of support for each gap value.
	real_t highestConfidence = 0.0;
	int offsetPos = 0;
	for (int i = 0; i < numOnsets; ++i)
	{
		int pos = wrappedPos[i];
		real_t confidence = GapConfidence(gapdata, 0, pos, interval);
		int offbeatPos = (pos + interval / 2) % interval;
		confidence += GapConfidence(gapdata, 0, offbeatPos, interval) * 0.5;

		if (confidence > highestConfidence)
		{
			highestConfidence = confidence;
			offsetPos = pos;
		}
	}

	return (real_t)offsetPos / (real_t)samplerate;
}

// Compares each offset to its corresponding offbeat value, and selects the most promising one.
static real_t AdjustForOffbeats(SerializedTempo* data, real_t offset, real_t bpm)
{
	int samplerate = data->samplerate;
	int numFrames = data->numFrames;

	// Create a slope representation of the waveform.
	real_t* slopes = AlignedMalloc(real_t, numFrames);
	ComputeSlopes(data->samples, slopes, numFrames, samplerate);

	// Determine the offbeat sample position.
	real_t secondsPerBeat = 60.0 / bpm;
	real_t offbeat = offset + secondsPerBeat * 0.5;
	if (offbeat > secondsPerBeat) offbeat -= secondsPerBeat;

	// Calculate the support for both sample positions.
	real_t end = (real_t)numFrames;
	real_t interval = secondsPerBeat * samplerate;
	real_t posA = offset * samplerate, sumA = 0.0;
	real_t posB = offbeat * samplerate, sumB = 0.0;
	for (; posA < end && posB < end; posA += interval, posB += interval)
	{
		sumA += slopes[(int)posA];
		sumB += slopes[(int)posB];
	}
	AlignedFree(slopes);

	// Return the offset with the highest support.
	return (sumA >= sumB) ? offset : offbeat;
}

// Selects the best offset value for each of the BPM candidates.
static void CalculateOffset(SerializedTempo* data, Onset* onsets, int numOnsets)
{
	auto& tempo = data->result;
	int samplerate = data->samplerate;

	// Create gapdata buffers for testing.
	real_t maxInterval = 0.0;
	for (auto& t : tempo) maxInterval = max(maxInterval, samplerate * 60.0 / t.bpm);
	GapData gapdata(1, (int)(maxInterval + 1.0), 1, numOnsets, onsets);

	// Fill in onset values for each BPM.
	for (auto& t : tempo)
		t.offset = GetBaseOffsetValue(gapdata, samplerate, t.bpm);

	// Test all onsets against their offbeat values, pick the best one.
	for (auto& t : tempo)
		t.offset = AdjustForOffbeats(data, t.offset, t.bpm);
}

// ================================================================================================
// BPM testing wrapper class

static const char* sProgressText[]
{
	"[1/6] Looking for onsets",
	"[2/6] Scanning intervals",
	"[3/6] Refining intervals",
	"[4/6] Selecting BPM values",
	"[5/6] Calculating offsets",
	"BPM detection results"
};

class TempoDetector  //: public TempoDetector, public BackgroundThread
{
    public:
        TempoDetector(Samples s, double time, double len);
		TempoDetector(Vector<Onset>* onsets, int sr);
        ~TempoDetector();

        void exec();

        // bool hasSamples() { return (myData.samples != nullptr); }
        // const char* getProgress() const { return sProgressText[myData.progress]; }
        // bool hasResult() const { return isDone(); }
        const Vector<TempoResult>& getResult() const { return myData.result; }

    private:
        SerializedTempo myData;
};

TempoDetector::TempoDetector(Samples s, double time, double len)
{
	// printf("%f, %f\n", time, len);
	auto& music = s;

	// Check if the number of frames is non-zero.
	int firstFrame = max(0, (int)(time * music.getFrequency()));
	int numFrames = max(0, (int)(len * music.getFrequency()));
	numFrames = min(numFrames, music.getNumFrames() - firstFrame);
	if (numFrames <= 0)
	{
		PrintOut("There is no audio selected to perform BPM detection on.");
		return;
	}

	this->myData = SerializedTempo();

	// myData.terminate = &myTerminateFlag;
	
	myData.numThreads = ParallelThreads::concurrency();
	myData.numFrames = numFrames;
	myData.samplerate = music.getFrequency();

	myData.samples = AlignedMalloc(float, numFrames);
	if (!myData.samples)
	{
		PrintErr("Something bad happened...");
		return;
	}

	// Copy the input samples.
	const short* l = music.samplesL() + firstFrame;
	const short* r = music.samplesR() + firstFrame;
	for (int i = 0; i < numFrames; ++i, ++l, ++r)
	{
		myData.samples[i] = (float)((int)*l + (int)*r) / 65536.0f;
	}
}

TempoDetector::TempoDetector(Vector<Onset>* onsets, int sr)
{
	this->myData = SerializedTempo();

	myData.numThreads = 1;
	myData.onsets = onsets;
	myData.samples = nullptr;
	myData.samplerate = sr;
}

TempoDetector::~TempoDetector()
{
	if (myData.samples != nullptr)
		AlignedFree(myData.samples);
}

void TempoDetector::exec()
{
	SerializedTempo* data = &myData;

	if (data->onsets != nullptr) {
		//printf("%d\n", data->onsets->size());
		CalculateBPM(data, data->onsets->data(), data->onsets->size());
		MarkProgress(4, "Find BPM");
		return;
	}

	// Run the aubio onset tracker to find note onsets.
	Vector<Onset> onsets;
	FindOnsets(data->samples, data->samplerate, data->numFrames, 1, &onsets);

	MarkProgress(1, "Find onsets");

    // Calculate the strength of each onset.
	/*
	for (int i = 0; i < min(onsets.size(), 100); ++i)
	{
		int a = max(0, onsets[i].pos - 100);
		int b = min(data->numFrames, onsets[i].pos + 100);
		float v = 0.0f;
		for (int j = a; j < b; ++j)
		{
			v += abs(data->samples[j]);
		}
		v /= (float)max(1, b - a);
		onsets[i].strength = v;
	}
	*/

	// Find BPM values.
	CalculateBPM(data, onsets.data(), onsets.size());
	MarkProgress(4, "Find BPM");

	// Find offset values.
	CalculateOffset(data, onsets.data(), onsets.size());
	MarkProgress(5, "Find offsets");
}

}; // namespace find_tempo_cpp
using namespace find_tempo_cpp;

Samples::Samples(aubio_source_t* aud, int hop_size) {
    fmat_t* buf = new_fmat(2, hop_size);  // create matrix
    unsigned int read = 0;  // bytes read each loop
    unsigned int total = 0;  // total bytes read
    
    // set class variables
    frame_len = aubio_source_get_duration(aud);
	// printf("%d\n", frame_len);
    freq = aubio_source_get_samplerate(aud);
	// printf("%d\n", freq);
    
    // split matrix into short arrays, left[] and right[]
    left = AlignedMalloc(short, frame_len);
    right = AlignedMalloc(short, frame_len);
	// mono = AlignedMalloc(float, frame_len);
    
    int leftchan = 0;
    int rightchan = 1;
    do {
		// printf("%d\n", total);
        aubio_source_do_multi(aud, buf, &read);
        for (int i = 0; i < read; i++) {
            left[total+i] = (short)(buf->data[leftchan][i] * 32767);  // float -> short, may have to change based on what the actual range is
            right[total+i] = (short)(buf->data[rightchan][i] * 32767);
			// mono[i] = buf->data[leftchan][total+i] + buf->data[rightchan][total+i];
        }
        total += read;
    } while (read == hop_size);
    
    del_fmat(buf);
}

// Samples::~Samples() {
//     AlignedFree(left);
//     AlignedFree(right);
// }

void get_tempo(double* onsetsPtr, long freq) {
    
}

}; // namespace vortex

int main(int argc, char** argv) {
	using namespace vortex;

    if (argc < 2) {
        PrintErr("no arguments found");
        PrintErr("usage: " + string(argv[0]) + " <source_path> [start=0.0] [duration=60.0] [hop_size=256]\n");
		PrintErr("batch usage: " + string(argv[0]) + " --batch <source_folder> <output_csv>\n");
        return 2;
    }

	if (string(argv[1]) == "--batch") {
		string pathname = string(argv[2]);

		ofstream outfile;
		outfile.open(string(argv[3]));

		for (const auto& entry : fs::directory_iterator(pathname)) {
			string fn = entry.path().string();
			string id = fn.substr(0, '.');

			string artist;
			string title;
			int samplerate;
			string method;

			ifstream input(fn);
			Vector<Onset> onsets;

			if (input) {
				getline(input, artist);
				//printf("%s\n", artist.c_str());
				getline(input, title);
				//printf("%s\n", title.c_str());
				input >> samplerate;
				getline(input, method);
				getline(input, method);
				//printf("%s\n", method.c_str());
				double d;
				while (input >> d) {
					//printf("%f\n", d);
					onsets.append(Onset((int)d, 1.0));
				}
			} else { PrintErr("This text file is empty?"); continue; }

			PrintOut(artist);
			PrintOut(title);

			TempoDetector tempo = TempoDetector(&onsets, samplerate);
			tempo.exec();
			
			TempoResults results = tempo.getResult();
			// loop through results, output to text file as csv
			int i = 0;
			#define QUOTE(A) "\"" + A + "\""
			#define DELIM ","
			// fix broken csv with regex: (\D|\d{1,4})\x0D, replace with \1
			// there are also broken quotes when the actual song title has double quotes so uhhh
			// (^.+?,)("[^,"]*)("[^,]*)(")([^,"]*") replace with \1\2\\\3\\\4\5
			// EXCEPT IT'S DOUBLE DOUBLE QUOTES NOT BACKSLASH QUOTE IN CSV, WHAT
			// fix the replace regex to be better; right now you can just replace \" with "" after the above regex replacement
			for (TempoResult result : results) {
				PrintStream(outfile, QUOTE(artist) + DELIM + QUOTE(title) + DELIM + QUOTE(id) + DELIM + QUOTE(method) + DELIM + to_string(++i) + DELIM + to_string(result.bpm) + DELIM + to_string(result.offset) + DELIM + to_string(result.fitness));
			}
			outfile.flush();
			onsets.clear();
		}

		outfile.close();

	} else {
		// load audio in 
		unsigned int samplerate = 0;
		unsigned int win_s = 1024; // window size
		unsigned int hop_size = win_s / 4;
		double start = 0.0;
		double duration = 60.0;
		
		char* source_path = argv[1];
		if (argc >= 3) start = atof(argv[2]);
		if (argc >= 4) duration = atof(argv[3]);
		if (argc >= 5) hop_size = atoi(argv[4]);
		
		aubio_source_t* s = new_aubio_source(source_path, samplerate, hop_size);
		if (!s) {
			PrintErr("Source is not valid, exiting...");
			del_aubio_source(s);
			aubio_cleanup();
			return 1;
		}
		
		Samples music = Samples(s, hop_size);
		MarkProgress(0, "Read audio samples");
		
		// add Samples to function arguments
		TempoDetector tempo = TempoDetector(music, start, min(duration, (double)aubio_source_get_duration(s) / samplerate));
		tempo.exec();
		
		TempoResults results = tempo.getResult();
		// loop through results, print result.bpm, result.offset and result.fitness
		for (TempoResult result : results)
			PrintOut("[RESULT] " + to_string(result.bpm) + " BPM, offset @ " + to_string(result.offset) + " sec, fitness " + to_string(result.fitness));
		// this function is bad and should be fixed

		// delete tempo;
		// delete music;

		del_aubio_source(s);
		aubio_cleanup();
	}
	return 0;
}