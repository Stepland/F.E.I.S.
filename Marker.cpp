//
// Created by Syméon on 17/08/2017.
//

#include "Marker.h"

Marker::Marker() {
    for (auto& folder : std::filesystem::directory_iterator("assets/textures/markers/")) {
        if (validMarkerFolder(folder.path())) {
        	initFromFolder(folder);
        	return;
        }
    }
    throw std::runtime_error("No valid marker found");
}

Marker::Marker(std::filesystem::path folder) {
	if (validMarkerFolder(folder)) {
		initFromFolder(folder);
		return;
	}
	std::stringstream err;
	err << "Invalid marker folder : " << folder.string();
	throw std::runtime_error(err.str());
}

std::optional<std::reference_wrapper<sf::Texture>> Marker::getSprite(MarkerEndingState state, float seconds) {
	std::ostringstream frameName;
	int frame = static_cast<int>((seconds*30.f+16.f));
	if (frame >= 0 and frame <= 15) {
		frameName << "ma" << std::setfill('0') << std::setw(2) << frame;
		std::string frameStr = frameName.str();
		return textures[frameName.str()];
	} else {
		if (state == MarkerEndingState_MISS) {
			if (frame >= 16 and frame <= 23) {
				frameName << "ma" << std::setfill('0') << std::setw(2) << frame;
				return textures[frameName.str()];
			}
		} else if (frame >= 16 and frame <= 31) {
			switch (state) {
				case MarkerEndingState_EARLY:
					frameName << "h1";
					break;
				case MarkerEndingState_GOOD:
					frameName << "h2";
					break;
				case MarkerEndingState_GREAT:
					frameName << "h3";
					break;
				case MarkerEndingState_PERFECT:
					frameName << "h4";
					break;
				default:
					return {};
			}
			frameName << std::setfill('0') << std::setw(2) << frame-16;
			return textures[frameName.str()];
		}
	}
	return {};
}

const std::map<std::string, sf::Texture> &Marker::getTextures() const {
	return textures;
}

bool Marker::validMarkerFolder(std::filesystem::path folder) {

    std::stringstream filename;
    // ma00 ~ ma23
    for (int i = 0; i < 24; ++i) {
		filename.str("");
        filename << "ma" << std::setw(2) << std::setfill('0') << i << ".png";
        std::string s_filename = filename.str();
        if (not std::filesystem::exists(folder/filename.str())) {
        	return false;
        }
    }
	// h100 ~ h115  +  h200 ~ h215  +   h300 ~ h315  +  h400 ~ h415
	for (int j = 1; j <= 4; ++j) {
		for (int i = 0; i < 16; ++i) {
			filename.str("");
			filename << "h" << 100*j + i << ".png";
			std::string s_filename = filename.str();
			if (not std::filesystem::exists(folder/filename.str())) {
				return false;
			}
		}
	}
	return true;
}

void Marker::initFromFolder(std::filesystem::path folder) {

	textures.clear();
	path = folder;

	// Chargement des h100~115 / h200~215 / h300~h315 / h400~415
	for (int sup = 1; sup<=4; sup++) {
		for (int i = 0; i <= 15; i++) {
			sf::Texture tex;
			if (!tex.loadFromFile(path.string() + "/h" + std::to_string(i+100*sup) + ".png")) {
				std::stringstream err;
				err << "Unable to load marker " << folder << "\nfailed on image" << (path.string() + "/h" + std::to_string(i+100*sup) + ".png");
				throw std::runtime_error(err.str());
			}
			tex.setSmooth(true);
			textures.insert({"h" + std::to_string(i+100*sup), tex});
		}
	}

	// Chargement de ma00~23
	for (int i = 0; i <= 23; i++) {

		sf::Texture tex;
		std::string fichier;
		if ( i < 10 ) {
			fichier = "ma0"
					  + std::to_string(i);
		} else {
			fichier = "ma"
					  + std::to_string(i);
		}

		if (!tex.loadFromFile(path.string()+"/"+fichier+".png")) {
			std::stringstream err;
			err << "Unable to load marker " << folder << "\nfailed on image" << (path.string()+"/"+fichier+".png");
			throw std::runtime_error(err.str());
		}
		tex.setSmooth(true);
		textures.insert({fichier, tex});
	}
}

/*
sf::Texture Marker::getSprite(MarkerEndingState state, int frame) {

	int lower;
	int upper;
	switch(state) {
		case MarkerEndingState_MISS:
			lower = 16;
			upper = 32;
			break;
		default:
			lower = 0;
			upper = 15;
	}

	if (!(frame >= lower and frame <= upper)) {
		std::cerr << "Requested access to a non-existent marker frame : " << frame;
		throw std::runtime_error("Requested access to a non-existent marker frame : " +std::to_string(frame));
	}

	std::string tex_key;
	switch (state) {
		case MarkerEndingState_MISS:
			tex_key += "ma";
			break;
		case MarkerEndingState_EARLY:
			tex_key += "h1";
			break;
		case MarkerEndingState_GOOD:
			tex_key += "h2";
			break;
		case MarkerEndingState_GREAT:
			tex_key += "h3";
			break;
		case MarkerEndingState_PERFECT:
			tex_key += "h4";
			break;
	}
	if (frame < 10) {
		tex_key += "0";
	}

	return textures[tex_key+std::to_string(frame)];

}
 */
