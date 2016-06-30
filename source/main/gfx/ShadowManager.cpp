/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "ShadowManager.h"

#include "Ogre.h"
#include "OgreTerrain.h"

#include "Settings.h"

using namespace Ogre;

ShadowManager::ShadowManager()
{
	PSSM_Shadows.mPSSMSetup.setNull();
	PSSM_Shadows.mDepthShadows = false;
	PSSM_Shadows.ShadowsTextureNum = 3;
	PSSM_Shadows.Quality = ISETTING("Shadows Quality", 2); //0 = Low quality, 1 = mid, 2 = hq, 3 = ultra
}

ShadowManager::~ShadowManager()
{
}

void ShadowManager::loadConfiguration()
{
	Ogre::String s = SSETTING("Shadow technique", "Parallel-split Shadow Maps");
	if (s == "Texture shadows")
		ShadowsType = SHADOWS_TEXTURE;
	else if (s == "Parallel-split Shadow Maps")
		ShadowsType = SHADOWS_PSSM;
	else
		ShadowsType = SHADOWS_NONE;

	updateShadowTechnique();
}

int ShadowManager::updateShadowTechnique()
{
	float shadowFarDistance = FSETTING("SightRange", 2000);
	//float scoef = 0.12;
	//gEnv->sceneManager->setShadowColour(Ogre::ColourValue(0.563 + scoef, 0.578 + scoef, 0.625 + scoef));

	if (ShadowsType == SHADOWS_TEXTURE)
	{
		gEnv->sceneManager->setShadowFarDistance(shadowFarDistance);
		processTextureShadows();
	}
	else if (ShadowsType == SHADOWS_PSSM)
	{
		processPSSM();
		
	}
	return 0;
}

void ShadowManager::processTextureShadows()
{

}

void ShadowManager::processPSSM()
{
	gEnv->sceneManager->setShadowFarDistance(60.0f);
	gEnv->sceneManager->setShadowTextureCasterMaterial("RoR_ShadowsCaster");
	gEnv->sceneManager->setShadowDirectionalLightExtrusionDistance(299.0f);
	gEnv->sceneManager->setShadowFarDistance(350.0f);
	gEnv->sceneManager->setShadowTextureCount(PSSM_Shadows.ShadowsTextureNum);

	gEnv->sceneManager->setShadowTextureSelfShadow(true);
	gEnv->sceneManager->setShadowCasterRenderBackFaces(true);

	//Caster is set via materials
	gEnv->sceneManager->setShadowTextureCasterMaterial("Ogre/shadow/depth/caster");


	if (PSSM_Shadows.Quality == 3)
	{
		gEnv->sceneManager->setShadowTextureConfig(0, 4096, 4096, PF_FLOAT32_R);
		gEnv->sceneManager->setShadowTextureConfig(1, 3072, 3072, PF_FLOAT32_R);
		gEnv->sceneManager->setShadowTextureConfig(2, 2048, 2048, PF_FLOAT32_R);
		PSSM_Shadows.lambda = 0.965f;
	}
	else if (PSSM_Shadows.Quality == 2)
	{
		gEnv->sceneManager->setShadowTextureConfig(0, 3072, 3072, PF_FLOAT32_R);
		gEnv->sceneManager->setShadowTextureConfig(1, 2048, 2048, PF_FLOAT32_R);
		gEnv->sceneManager->setShadowTextureConfig(2, 2048, 2048, PF_FLOAT32_R);
		PSSM_Shadows.lambda = 0.97f;
	}
	else if(PSSM_Shadows.Quality == 1)
	{
		gEnv->sceneManager->setShadowTextureConfig(0, 2048, 2048, PF_FLOAT32_R);
		gEnv->sceneManager->setShadowTextureConfig(1, 1024, 1024, PF_FLOAT32_R);
		gEnv->sceneManager->setShadowTextureConfig(2, 1024, 1024, PF_FLOAT32_R);
		PSSM_Shadows.lambda = 0.975f;
	}
	else 
	{
		gEnv->sceneManager->setShadowTextureConfig(0, 1024, 1024, PF_FLOAT32_R);
		gEnv->sceneManager->setShadowTextureConfig(1, 1024, 1024, PF_FLOAT32_R);
		gEnv->sceneManager->setShadowTextureConfig(2,  512,  512, PF_FLOAT32_R);
		PSSM_Shadows.lambda = 0.98f;
	}

		//Send split info to managed materials
		setManagedMaterialSplitPoints(pssmSetup->getSplitPoints());
}

void ShadowManager::updatePSSM()
{
	//Ugh what here?
}
	{
		Ogre::PSSMShadowCameraSetup* pssmSetup = static_cast<Ogre::PSSMShadowCameraSetup*>(PSSM_Shadows.mPSSMSetup.get());
		matProfile->setReceiveDynamicShadowsDepth(true);
		matProfile->setReceiveDynamicShadowsLowLod(false);
	}

	GpuSharedParametersPtr p = GpuProgramManager::getSingleton().getSharedParameters("pssm_params");
	p->setNamedConstant("pssmSplitPoints", splitPoints);

}
