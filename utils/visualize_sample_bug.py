from dataclasses import dataclass

import matplotlib.pyplot as plt

@dataclass
class BadSample:
    buffer: tuple[int, int]
    sample: tuple[int, int]
    deoverlapped_end: int
    next_sample: int | None

    @classmethod
    def from_dump(cls, dump: str):
        nums = [int(s.strip()) for s in dump.split(",")]
        if len(nums) == 5:
            return cls(
                buffer=(nums[0], nums[1]),
                sample=(nums[2], nums[3]),
                deoverlapped_end=nums[4],
                next_sample=None,
            )
        elif len(nums) == 6:
            return cls(
                buffer=(nums[0], nums[1]),
                sample=(nums[2], nums[3]),
                deoverlapped_end=nums[4],
                next_sample=nums[5],
            )
        else:
            raise ValueError("Couldn't parse dump")


def show_bad_sample_plot(bad_sample: BadSample):
    fig, ax = plt.subplots()
    ax.broken_barh([(bad_sample.buffer[0], bad_sample.buffer[1] - bad_sample.buffer[0])], (9, 2), facecolors="tab:blue")
    ax.broken_barh([(bad_sample.sample[0], bad_sample.sample[1] - bad_sample.sample[0])], (7, 2), facecolors="tab:green")
    ax.broken_barh([(bad_sample.deoverlapped_end, 100)], (6.5, 1), facecolors="tab:red")
    if bad_sample.next_sample is not None:
        ax.broken_barh([(bad_sample.next_sample, 100)], (5, 2), facecolors="tab:orange")
    ax.set_ylim(4, 12)
    ax.set_xlabel("samples")
    ax.set_yticks([10, 8, 6], labels=["buffer", "sample", "next sample"])
    plt.show()

if __name__ == "__main__":
    import sys

    bad_sample = BadSample.from_dump(" ".join(sys.argv[1:]))
    show_bad_sample_plot(bad_sample)