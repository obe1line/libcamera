/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2021-2022, Ideas On Board
 *
 * RkISP1 Black Level Correction control
 */

#include "blc.h"

#include <libcamera/base/log.h>

#include "libcamera/internal/yaml_parser.h"

/**
 * \file blc.h
 */

namespace libcamera {

namespace ipa::rkisp1::algorithms {

/**
 * \class BlackLevelCorrection
 * \brief RkISP1 Black Level Correction control
 *
 * The pixels output by the camera normally include a black level, because
 * sensors do not always report a signal level of '0' for black. Pixels at or
 * below this level should be considered black. To achieve that, the RkISP BLC
 * algorithm subtracts a configurable offset from all pixels.
 *
 * The black level can be measured at runtime from an optical dark region of the
 * camera sensor, or measured during the camera tuning process. The first option
 * isn't currently supported.
 */

LOG_DEFINE_CATEGORY(RkISP1Blc)

BlackLevelCorrection::BlackLevelCorrection()
	: tuningParameters_(false)
{
}

/**
 * \copydoc libcamera::ipa::Algorithm::init
 */
int BlackLevelCorrection::init([[maybe_unused]] IPAContext &context,
			       const YamlObject &tuningData)
{
	std::optional<int16_t> levelRed = tuningData["R"].get<int16_t>();
	std::optional<int16_t> levelGreenR = tuningData["Gr"].get<int16_t>();
	std::optional<int16_t> levelGreenB = tuningData["Gb"].get<int16_t>();
	std::optional<int16_t> levelBlue = tuningData["B"].get<int16_t>();
	bool tuningHasLevels = levelRed && levelGreenR && levelGreenB && levelBlue;

	auto blackLevel = context.camHelper->blackLevel();
	if (!blackLevel) {
		/*
		 * Not all camera sensor helpers have been updated with black
		 * levels. Print a warning and fall back to the levels from the
		 * tuning data to preserve backward compatibility. This should
		 * be removed once all helpers provide the data.
		 */
		LOG(RkISP1Blc, Warning)
			<< "No black levels provided by camera sensor helper"
			<< ", please fix";

		blackLevelRed_ = levelRed.value_or(4096);
		blackLevelGreenR_ = levelGreenR.value_or(4096);
		blackLevelGreenB_ = levelGreenB.value_or(4096);
		blackLevelBlue_ = levelBlue.value_or(4096);
	} else if (tuningHasLevels) {
		/*
		 * If black levels are provided in the tuning file, use them to
		 * avoid breaking existing camera tuning. This is deprecated and
		 * will be removed.
		 */
		LOG(RkISP1Blc, Warning)
			<< "Deprecated: black levels overwritten by tuning file";

		blackLevelRed_ = *levelRed;
		blackLevelGreenR_ = *levelGreenR;
		blackLevelGreenB_ = *levelGreenB;
		blackLevelBlue_ = *levelBlue;
	} else {
		blackLevelRed_ = *blackLevel;
		blackLevelGreenR_ = *blackLevel;
		blackLevelGreenB_ = *blackLevel;
		blackLevelBlue_ = *blackLevel;
	}

	tuningParameters_ = true;

	LOG(RkISP1Blc, Debug)
		<< "Black levels: red " << blackLevelRed_
		<< ", green (red) " << blackLevelGreenR_
		<< ", green (blue) " << blackLevelGreenB_
		<< ", blue " << blackLevelBlue_;

	return 0;
}

/**
 * \copydoc libcamera::ipa::Algorithm::prepare
 */
void BlackLevelCorrection::prepare([[maybe_unused]] IPAContext &context,
				   const uint32_t frame,
				   [[maybe_unused]] IPAFrameContext &frameContext,
				   rkisp1_params_cfg *params)
{
	if (frame > 0)
		return;

	if (!tuningParameters_)
		return;

	params->others.bls_config.enable_auto = 0;
	/* The rkisp1 uses 12bit based black levels. Scale down accordingly. */
	params->others.bls_config.fixed_val.r = blackLevelRed_ >> 4;
	params->others.bls_config.fixed_val.gr = blackLevelGreenR_ >> 4;
	params->others.bls_config.fixed_val.gb = blackLevelGreenB_ >> 4;
	params->others.bls_config.fixed_val.b = blackLevelBlue_ >> 4;

	params->module_en_update |= RKISP1_CIF_ISP_MODULE_BLS;
	params->module_ens |= RKISP1_CIF_ISP_MODULE_BLS;
	params->module_cfg_update |= RKISP1_CIF_ISP_MODULE_BLS;
}

REGISTER_IPA_ALGORITHM(BlackLevelCorrection, "BlackLevelCorrection")

} /* namespace ipa::rkisp1::algorithms */

} /* namespace libcamera */
