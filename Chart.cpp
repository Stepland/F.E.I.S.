//
// Created by Syméon on 25/08/2017.
//

#include "Chart.h"

int Chart::getResolution() const {
	return resolution;
}

void Chart::setResolution(int resolution) {
	if (resolution <= 0) {
		throw std::invalid_argument("Can't set a resolution of "+std::to_string(resolution));
	} else {
		this->resolution = resolution;
	}
}

Chart::Chart(const std::string &dif, int level, int resolution) : dif_name(dif),
                                                                  level(level),
                                                                  resolution(resolution),
                                                                  Notes() {
	if (resolution <= 0) {
		throw std::invalid_argument("Can't set a resolution of "+std::to_string(resolution));
	}
}
