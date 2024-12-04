/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi Ltd
 *
 * camera helper for Innodisk EVDM-OOM1 module (AP1302 ISP and AR1335 sensor)
 */

#include <assert.h>

#include "cam_helper.h"

using namespace RPiController;


// TODO: extend to support embedded metadata (see imx219 for example)

class CamHelperEvdmOOM1 : public CamHelper
{
public:
	CamHelperEvdmOOM1();
	uint32_t gainCode(double gain) const override;
	double gain(uint32_t gainCode) const override;
	void getDelays(int &exposureDelay, int &gainDelay,
		       int &vblankDelay, int &hblankDelay) const override;
	bool sensorEmbeddedDataPresent() const override;
	unsigned int hideFramesStartup() const override;
	unsigned int hideFramesModeSwitch() const override;
	unsigned int mistrustFramesStartup() const override;
	unsigned int mistrustFramesModeSwitch() const override;

private:
	/*
	 * Smallest difference between the frame length and integration time,
	 * in units of lines.
	 */
	static constexpr int frameIntegrationDiff = 22;
};

CamHelperEvdmOOM1::CamHelperEvdmOOM1()
	: CamHelper({}, frameIntegrationDiff)
{
}

uint32_t CamHelperEvdmOOM1::gainCode(double gain) const
{
	return static_cast<uint32_t>(gain * 16.0);
}

double CamHelperEvdmOOM1::gain(uint32_t gainCode) const
{
	return static_cast<double>(gainCode) / 16.0;
}

void CamHelperEvdmOOM1::getDelays(int &exposureDelay, int &gainDelay,
				int &vblankDelay, int &hblankDelay) const
{
	exposureDelay = 2;
	gainDelay = 2;
	vblankDelay = 2;
	hblankDelay = 2;
}

bool CamHelperEvdmOOM1::sensorEmbeddedDataPresent() const
{
	// for now ignore embedded data
	return false;
}

unsigned int CamHelperEvdmOOM1::hideFramesStartup() const
{
	/*
	 * On startup, we get a couple of under-exposed frames which
	 * we don't want shown.
	 */
	return 2;
}

unsigned int CamHelperEvdmOOM1::hideFramesModeSwitch() const
{
	/*
	 * After a mode switch, we get a couple of under-exposed frames which
	 * we don't want shown.
	 */
	return 2;
}

unsigned int CamHelperEvdmOOM1::mistrustFramesStartup() const
{
	/*
	 * First couple of frames are under-exposed and are no good for control
	 * algos.
	 */
	return 2;
}

unsigned int CamHelperEvdmOOM1::mistrustFramesModeSwitch() const
{
	/*
	 * First couple of frames are under-exposed even after a simple
	 * mode switch, and are no good for control algos.
	 */
	return 2;
}

static CamHelper *create()
{
	return new CamHelperEvdmOOM1();
}

static RegisterCamHelper reg("evdmoom1", &create);
