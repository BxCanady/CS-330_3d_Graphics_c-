///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
   const char* g_ViewPositionName = "viewPosition";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
   m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  ApplySceneLighting()
 *
 *  This method sets the lighting uniforms shared by the scene.
 ***********************************************************/
void SceneManager::ApplySceneLighting()
{
	if (NULL == m_pShaderManager)
	{
		return;
	}

	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Shared material for the scene so the textured plane can reflect light.
	m_pShaderManager->setVec3Value("material.ambientColor", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setFloatValue("material.ambientStrength", 0.25f);
	m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.85f, 0.85f, 0.85f));
	m_pShaderManager->setFloatValue("material.shininess", 32.0f);

	// Warm desk lamp light.
	m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(7.4f, 2.1f, 1.0f));
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.10f, 0.08f, 0.04f));
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(1.0f, 0.87f, 0.68f));
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(1.0f, 0.96f, 0.85f));
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 48.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.90f);

	// Cool overhead fill light to keep the far side visible.
	m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(-4.0f, 8.0f, 6.0f));
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.05f, 0.06f, 0.08f));
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(0.60f, 0.70f, 0.85f));
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(0.70f, 0.78f, 0.92f));
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.55f);

	// Soft front light for the camera side of the scene.
	m_pShaderManager->setVec3Value("lightSources[2].position", glm::vec3(0.0f, 4.0f, 12.0f));
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", glm::vec3(0.03f, 0.03f, 0.03f));
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", glm::vec3(0.45f, 0.45f, 0.48f));
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", glm::vec3(0.55f, 0.55f, 0.60f));
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.25f);

	// Very dim ambient fill so nothing drops fully into shadow.
	m_pShaderManager->setVec3Value("lightSources[3].position", glm::vec3(0.0f, 10.0f, 0.0f));
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", glm::vec3(0.02f, 0.02f, 0.02f));
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", glm::vec3(0.20f, 0.20f, 0.20f));
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", glm::vec3(0.20f, 0.20f, 0.20f));
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 8.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.10f);
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glDeleteTextures(1, &m_textureIDs[i].ID);
		m_textureIDs[i].ID = 0;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);
		m_pShaderManager->setVec4Value(g_ColorValueName, glm::vec4(1.0f));

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetShaderTextureTint()
 *
 *  This method is used for setting a textured object tint.
 ***********************************************************/
void SceneManager::SetShaderTextureTint(
	std::string textureTag,
	glm::vec4 tintColor)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);
		m_pShaderManager->setVec4Value(g_ColorValueName, tintColor);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
   bool bReturn = false;

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadSphereMesh();

	// load and bind textures used by the scene
	bReturn = CreateGLTexture("../../Utilities/textures/knife_handle.jpg", "deskTexture");
	if (bReturn == false)
	{
		std::cout << "Failed to load desk texture" << std::endl;
	}
	bReturn = CreateGLTexture("../../Utilities/textures/stainless.jpg", "bezelTexture");
	if (bReturn == false)
	{
		std::cout << "Failed to load bezel texture" << std::endl;
	}
  bReturn = CreateGLTexture("../../Utilities/textures/keyboard.jpg", "keyboardTexture");
	if (bReturn == false)
	{
		bReturn = CreateGLTexture("../../Utilities/textures/keyboard.png", "keyboardTexture");
	}
	if (bReturn == false)
	{
		std::cout << "Failed to load keyboard texture" << std::endl;
	}
	bReturn = CreateGLTexture("../../Utilities/textures/stainless_end.jpg", "lampTexture");
	if (bReturn == false)
	{
		bReturn = CreateGLTexture("../../Utilities/textures/stainless_end.jpg", "lampTexture");
	}
	if (bReturn == false)
	{
		std::cout << "Failed to load keyboard texture" << std::endl;
	}
	bReturn = CreateGLTexture("../../Utilities/textures/screentexture.png", "screenTexture");
	if (bReturn == false)
	{
		std::cout << "Failed to load screen texture" << std::endl;
	}

	BindGLTextures();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
    ApplySceneLighting();

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// ==================== BACKGROUND WALL ====================
// Pearl white (slight cool tint)
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 5.0f, -5.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
  SetShaderColor(0.87f, 0.78f, 0.52f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	// ==================== DESK SURFACE ====================
	// Champagne (warm off-white)
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
  SetShaderTexture("deskTexture");
	SetTextureUVScale(2.5f, 1.5f);
	m_basicMeshes->DrawPlaneMesh();

	// Group offset for centering and moving forward on desk
	glm::vec3 groupOffset = glm::vec3(1.0f, 0.0f, 5.5f); // X: center, Z: move forward
			

	
	// Bezel - Top frame
	SetTransformations(glm::vec3(5.1f, 0.3f, 0.2f), 0.0f, 0.0f, 0.0f, glm::vec3(0.0f, 4.7f, -3.05f) + groupOffset);
  SetShaderTexture("bezelTexture");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();
	
	// Bezel - Bottom frame
	SetTransformations(glm::vec3(5.1f, 0.3f, 0.2f), 0.0f, 0.0f, 0.0f, glm::vec3(0.0f, 2.3f, -3.05f) + groupOffset);
  SetShaderTexture("bezelTexture");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();
	
	// Bezel - Left frame
	SetTransformations(glm::vec3(0.3f, 2.8f, 0.2f), 0.0f, 0.0f, 0.0f, glm::vec3(-2.55f, 3.5f, -3.05f) + groupOffset);
  SetShaderTexture("bezelTexture");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();
	
	// Bezel - Right frame
	SetTransformations(glm::vec3(0.3f, 2.8f, 0.2f), 0.0f, 0.0f, 0.0f, glm::vec3(2.55f, 3.5f, -3.05f) + groupOffset);
  SetShaderTexture("bezelTexture");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// MONITOR SCREEN
	SetTransformations(
		glm::vec3(5.0f, 2.2f, 0.05f),
		0.0f,
		0.0f,
		0.0f,
		glm::vec3(0.0f, 3.5f, -2.95f) + groupOffset);

      SetShaderTextureTint("screenTexture", glm::vec4(0.62f, 0.62f, 0.68f, 1.0f));
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Dell badge
	SetTransformations(
		glm::vec3(0.18f, 0.18f, 0.03f),
		0.0f,
		0.0f,
		0.0f,
		glm::vec3(0.0f, 2.45f, -2.90f) + groupOffset);

	SetShaderColor(0.80f, 0.80f, 0.80f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// Stand
	SetTransformations(glm::vec3(0.35f, 1.2f, 0.25f), 0.0f, 0.0f, 0.0f, glm::vec3(0.0f, 1.5f, -3.0f) + groupOffset);
	SetShaderColor(0.65f, 0.65f, 0.68f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Base
	SetTransformations(glm::vec3(1.8f, 0.15f, 1.0f), 0.0f, 0.0f, 0.0f, glm::vec3(0.0f, 0.75f, -3.0f) + groupOffset);
	SetShaderTexture("lampTexture");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Keyboard
	SetTransformations(glm::vec3(2.6f, 0.08f, 0.75f), 0.0f, 0.0f, 0.0f, glm::vec3(0.0f, 0.22f, -2.3f) + groupOffset);
  SetShaderTexture("keyboardTexture");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Computer mouse
	// Mouse body
    SetTransformations(glm::vec3(0.46f, 0.14f, 0.62f), 0.0f, 18.0f, 0.0f, glm::vec3(2.0f, 0.20f, 4.5f));
	SetShaderColor(0.12f, 0.12f, 0.14f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// Scroll wheel
  SetTransformations(glm::vec3(0.06f, 0.04f, 0.12f), 90.0f, 0.0f, 0.0f, glm::vec3(2.02f, 0.27f, 4.52f));
	SetShaderColor(0.20f, 0.20f, 0.22f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Book on desk
	// Cover
    SetTransformations(glm::vec3(1.9f, 0.10f, 1.3f), 0.0f, -20.0f, 0.0f, glm::vec3(-2.9f, 0.18f, 1.2f));
	SetShaderColor(0.22f, 0.30f, 0.70f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Pages
  SetTransformations(glm::vec3(1.78f, 0.075f, 1.18f), 0.0f, -20.0f, 0.0f, glm::vec3(-2.9f, 0.23f, 1.2f));
	SetShaderColor(0.96f, 0.95f, 0.90f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Spine accent
    SetTransformations(glm::vec3(0.14f, 0.105f, 1.30f), 0.0f, -20.0f, 0.0f, glm::vec3(-3.80f, 0.185f, 1.2f));
	SetShaderColor(0.14f, 0.20f, 0.52f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Desk lamp
// Base 
	SetTransformations(glm::vec3(0.95f, 0.10f, 0.95f), 0.0f, 0.0f, 0.0f, glm::vec3(7.4f, 0.15f, 1.0f));
  SetShaderTexture("lampTexture");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Vertical stem 
 SetTransformations(glm::vec3(0.12f, 1.60f, 0.12f), 0.0f, 0.0f, 0.0f, glm::vec3(7.4f, 0.45f, 1.0f));
  SetShaderTexture("lampTexture");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Lamp shade (cone) -> X shifted from 7.72f to 7.4f to match the stem center
        SetTransformations(glm::vec3(0.55f, 0.80f, 0.55f), 90.0f, 0.0f, 0.0f, glm::vec3(7.4f, 1.90f, 1.0f));
	SetShaderColor(0.96f, 0.96f, 0.98f, 1.0f);
	m_basicMeshes->DrawConeMesh();

	// Warm glow inside shade -> X shifted from 7.70f to 7.4f to match the stem center
        SetTransformations(glm::vec3(0.35f, 0.52f, 0.35f), 90.0f, 0.0f, 0.0f, glm::vec3(7.4f, 1.88f, 1.0f));
	SetShaderColor(1.0f, 0.92f, 0.45f, 0.55f);
	m_basicMeshes->DrawConeMesh();

	// Desk legs - four corners under the desk plane
	// Desk plane is centered at (0,0,0) with scale X=20, Z=10
	// place legs slightly inset from corners
	float legInsetX = 9.5f; // half-width (10) minus small inset
	float legInsetZ = 4.5f; // half-depth (5) minus small inset
	glm::vec3 legScale = glm::vec3(0.3f, 1.2f, 0.3f);
	float legHeight = legScale.y;
	// center Y is negative so legs sit under the desk surface
	float legCenterY = -legHeight * 0.7f;

	SetShaderColor(0.36f, 0.26f, 0.18f, 1.0f); // wood/darker color for legs

	// front-left
	SetTransformations(legScale, 0.0f, 0.0f, 0.0f, glm::vec3(-legInsetX, legCenterY, legInsetZ));
	m_basicMeshes->DrawBoxMesh();
	// front-right
	SetTransformations(legScale, 0.0f, 0.0f, 0.0f, glm::vec3(legInsetX, legCenterY, legInsetZ));
	m_basicMeshes->DrawBoxMesh();
	// back-left
	SetTransformations(legScale, 0.0f, 0.0f, 0.0f, glm::vec3(-legInsetX, legCenterY, -legInsetZ));
	m_basicMeshes->DrawBoxMesh();
	// back-right
	SetTransformations(legScale, 0.0f, 0.0f, 0.0f, glm::vec3(legInsetX, legCenterY, -legInsetZ));
	m_basicMeshes->DrawBoxMesh();

}