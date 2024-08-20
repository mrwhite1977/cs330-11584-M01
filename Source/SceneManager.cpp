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

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// clear all the allocated memory
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;

	// destroy the created OpenGL textures
	DestroyGLTextures();

	// clear the collection of defined materials
	m_objectMaterials.clear();
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
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
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
* LoadSceneTextures()
*
* This method is used for preparing the 3D scene by loading
* the shapes, textures in memory to support the 3D scene
* rendering
***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"C:/Users/miche/CS330Content/Projects/7-1_FinalProjectMilestones/Source/textures/green-shed.jpg",
		"shed");

	bReturn = CreateGLTexture(
		"C:/Users/miche/CS330Content/Projects/7-1_FinalProjectMilestones/Source/textures/yellow_roof.jpg",
		"roof");

	bReturn = CreateGLTexture(
		"C:/Users/miche/CS330Content/Projects/7-1_FinalProjectMilestones/Source/textures/grass.jpg",
		"grass");

	bReturn = CreateGLTexture(
		"C:/Users/miche/CS330Content/Projects/7-1_FinalProjectMilestones/Source/textures/stainless.jpg",
		"trough");

	bReturn = CreateGLTexture(
		"C:/Users/miche/CS330Content/Projects/7-1_FinalProjectMilestones/Source/textures/tractor-tire.jpg",
		"tractor");

	bReturn = CreateGLTexture(
		"C:/Users/miche/CS330Content/Projects/7-1_FinalProjectMilestones/Source/textures/tire-tread.jpg",
		"tread");

	bReturn = CreateGLTexture(
		"C:/Users/miche/CS330Content/Projects/7-1_FinalProjectMilestones/Source/textures/red-wagon.jpg",
		"trailer");
	
	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	goldMaterial.ambientStrength = 0.3f;
	goldMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	goldMaterial.shininess = 22.0;
	goldMaterial.tag = "metal";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL grassMaterial;
	grassMaterial.ambientColor = glm::vec3(0.2f, 0.3f, 0.4f);
	grassMaterial.ambientStrength = 0.3f;
	grassMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	grassMaterial.specularColor = glm::vec3(0.4f, 0.5f, 0.6f);
	grassMaterial.shininess = 1.0;
	grassMaterial.tag = "ground";

	m_objectMaterials.push_back(grassMaterial);

	OBJECT_MATERIAL rubberMaterial;
	rubberMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.3f);
	rubberMaterial.ambientStrength = 0.3f;
	rubberMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f);
	rubberMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.4f);
	rubberMaterial.shininess = 0.3;
	rubberMaterial.tag = "rubber";

	m_objectMaterials.push_back(rubberMaterial);
}
	
/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// pShaderManager->setBoolValue(g_UseLightingName, true);

	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Overhead light
	m_pShaderManager->setVec3Value("lightSources[0].direction", 0.0f, -1.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.05f);

	// Light in front-left of scene
	m_pShaderManager->setVec3Value("lightSources[1].position", -5.0f, 0.0f, -10.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.05f);

	// Light in front-right of scene
	m_pShaderManager->setVec3Value("lightSources[2].position", 5.0f, 0.0f, -10.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.05f);

	// Light in rear of scene
	m_pShaderManager->setVec3Value("lightSources[3].position", 0.0f, 0.0f, 10.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.05f);
	
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures for the 3D scene
	LoadSceneTextures();
	DefineObjectMaterials();
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadPyramid4Mesh();

}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	glm::mat4 scale;
	glm::mat4 rotation;
	glm::mat4 rotation2;
	glm::mat4 translation;
	glm::mat4 mode;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	// Render plane for ground
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.75, 0.75, 0.75, 1); // Grey color
	SetShaderTexture("grass");
	SetShaderMaterial("ground");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// Render background plane
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 9.0f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(0.196078, 0.196078, 1.0, 1); // Light blue

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/******************************************************************/

	// Render box for shed
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);  // Scale larger than other object in scene for proper persective

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 10.0f; // Rotated 10 degrees on Y-axis to match scene
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 2.0f, 0.0f);  // Positioned to left of origin to match scene

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(0, 1, 0, 1); // Green color
	SetShaderTexture("shed");
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	// Render pyramid for shed roof
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.5f, 3.9f, 3.5f);  // Scaling to match box to create shed

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 10.0f; // Rotated 10 degrees on Y-axis to match box
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 5.7f, 0.0f);  // Positioned above box to create roof

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(1, 1, 0, 1); // Yellow color
	SetShaderTexture("roof");
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPyramid4Mesh();
	/******************************************************************/

	// Render torus for water tank
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.5f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;  // Rotate X-axis to lay open-face against ground
	YrotationDegrees = 0.0f;  // Rotate Y-axis -5 degrees to apprear straight
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.0f, 1.0f, 5.0f); // Position to the left of shed

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(0.5, 0.5, 0.5, 1); // Gray color
	SetShaderTexture("trough");
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/******************************************************************/

	// Render cylinder for barrel
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 1.0f, 0.5f); // Scale to smaller size in comparison of shed

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f; 
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 7.5f);  // Position slightly right and in foreground of shed

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(1, 0.5, 0, 1); // Orange color
	SetShaderMaterial("rubber");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	// Render torus for tire on top of wagon
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.75f, 0.75f, 0.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -5.0f;
	YrotationDegrees = 170.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.8f, 2.4f, 3.5f); // Position to the right of scene and to be on top of wagon

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(0, 0, 0, 1); // Black color
	SetShaderTexture("tread");
	SetShaderMaterial("rubber");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/******************************************************************/

	// Render box to create wagon frame
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 2.0f, 1.0f);  

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f; // Rotated 45 degrees on Y-axis
	ZrotationDegrees = -25.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 1.0f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(1, 0, 0, 1); // Red color
	SetShaderTexture("trailer");
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	// Render cylinder for wagon wheel front-right
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f); // Scale to smaller size in comparison of shed

	// set the XYZ rotation for the mesh
	XrotationDegrees = -5.0f;
	YrotationDegrees = 105.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.2f, 0.6f, 3.4f);  // Position under wagon

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(0, 0, 0, 1); // black color
	SetShaderTexture("tread");
	SetShaderMaterial("rubber");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	// Render cylinder for wagon wheel back-right
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f); // Scale to smaller size in comparison of shed

	// set the XYZ rotation for the mesh
	XrotationDegrees = -5.0f;
	YrotationDegrees = 105.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.5f, 0.6f, 1.4f);  // Position under wagon

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(0, 0, 0, 1); // black color
	SetShaderTexture("tread");
	SetShaderMaterial("rubber");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	// Render cylinder for wagon wheel front-left
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f); // Scale to smaller size in comparison of shed

	// set the XYZ rotation for the mesh
	XrotationDegrees = -5.0f;
	YrotationDegrees = 105.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.2f, 0.6f, 4.3f);  // Position under wagon

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(0, 0, 0, 1); // black color
	SetShaderTexture("tread");
	SetShaderMaterial("rubber");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	// Render cylinder for wagon wheel back-left
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f); // Scale to smaller size in comparison of shed

	// set the XYZ rotation for the mesh
	XrotationDegrees = -5.0f;
	YrotationDegrees = 105.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.5f, 0.6f, 2.2f);  // Position under wagon

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(0, 0, 0, 1); // black color
	SetShaderTexture("tread");
	SetShaderMaterial("rubber");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/
}
