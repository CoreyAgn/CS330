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

/****************************************
* LoadSceneTextures()
* 
* Method used to prepare textures for 3D space
* by loading textures into memory
****************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	//Importing texture files
	
	bReturn = CreateGLTexture(
	"ProjectTextures/plastic.jpg",
	"plastic");

	bReturn = CreateGLTexture(
	"ProjectTextures/micmetal.jpg",
	"blackmetal");

	bReturn = CreateGLTexture(
	"ProjectTextures/miccap.jpg",
	"rubbercap");

	bReturn = CreateGLTexture(
	"ProjectTextures/micringconnection.jpg",
	"silverthread");

	bReturn = CreateGLTexture(
	"ProjectTextures/grille.jpg",
	"micgrille");

	bReturn = CreateGLTexture(
	"ProjectTextures/metalring.jpg",
	"silverring");

	bReturn = CreateGLTexture(
	"ProjectTextures/wood.jpg",
	"wood");

	bReturn = CreateGLTexture(
	"ProjectTextures/Pink.jpg",
	"pink");

	//Bind imported textures
	BindGLTextures();
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

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used to edit various material settings
 *  for objects in the scene
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{


	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	woodMaterial.ambientStrength = 0.3f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.shininess = 16.0;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	metalMaterial.ambientStrength = 0.1f;
	metalMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	metalMaterial.specularColor = glm::vec3(.5f, .5f, .5f);
	metalMaterial.shininess = 8.0;
	metalMaterial.tag = "metal";

	m_objectMaterials.push_back(metalMaterial);

	OBJECT_MATERIAL matteMaterial;
	matteMaterial.ambientColor = glm::vec3(0.25f, 0.25f, 0.25f);
	matteMaterial.ambientStrength = 0.5f;
	matteMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f);
	matteMaterial.specularColor = glm::vec3(.02f, .02f, .02f);
	matteMaterial.shininess = 1.0;
	matteMaterial.tag = "matte";

	m_objectMaterials.push_back(matteMaterial);
}


/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add light sources for the scene
 ***********************************************************/
void SceneManager::SetupSceneLights()
{

	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	m_pShaderManager->setVec3Value("lightSources[0].position", 2.0f, 0.0f, -2.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.3f, .3f, .3f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.7f, 0.7f, .7f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 2.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.2f);

	m_pShaderManager->setVec3Value("lightSources[1].position", 5.0f, 6.0f, 5.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", .2f, .2f, .0f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.6f, 0.6f, .8f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 64.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.2f);
	
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
	//load texture image files for 3D objects
	LoadSceneTextures();

	//add light sources to the scene
	SetupSceneLights();

	//define materials to used for objects
	DefineObjectMaterials();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();

}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	//Render the different objects for scene
	RenderPlane();
	RenderMicrophone();
	RenderClayBall();
	RenderEnergyDrink();
	RenderHeatGun();

}

/*************************************************
* RenderPlane
*
* This method renders the plane for the 3D scene
* ************************************************/
void SceneManager::RenderPlane()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** plane renders	***/

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

	//Assign texture and UV scaling
	SetShaderTexture("wood");
	SetTextureUVScale(3.0, 3.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
}

/*************************************************
* RenderMicrophone()
* 
* This method renders the shapes for the microphone object
* ************************************************/
void SceneManager::RenderMicrophone() 
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** cylinder renders ***/
	
	// microphone base
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.3f, .1f, 1.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 0.0f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Assign texture and UV scaling
	SetShaderTexture("blackmetal");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/


	// mic body
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.6f, 2.2f, .6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, .8f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Assign texture and UV scaling
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//microphone grille
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.6f, 1.3f, .6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 3.0f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	
	//Assign texture and UV scaling
	SetShaderTexture("micgrille");
	SetTextureUVScale(2.0, 2.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/


	// mic cap top
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.6f, .4f, .6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 4.3f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Assign texture and UV scaling
	SetShaderTexture("rubbercap");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("matte");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/


	// mic cap bottom
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.6f, .2f, .6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, .6f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Assign texture and UV scaling
	SetShaderTexture("rubbercap");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("matte");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/


	// top right mic ring to mic body connection
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.08f, .5f, .08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 155.0f;
	ZrotationDegrees = -45.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.6f, 2.4f, 3.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Assign texture and UV scaling
	SetShaderTexture("silverthread");
	SetTextureUVScale(5.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/


	// bottom right mic ring to mic body connection
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.08f, .5f, .08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 155.0f;
	ZrotationDegrees = 45.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.3f, 1.15f, 3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Assign texture and UV scaling
	SetShaderTexture("silverthread");
	SetTextureUVScale(5.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/


	// bottom left mic ring to mic body connection
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.08f, .5f, .08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 45.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.55f, 1.15f, 3.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Assign texture and UV scaling
	SetShaderTexture("silverthread");
	SetTextureUVScale(5.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/


	// top left mic ring to mic body connection
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.08f, .5f, .08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -155.0f;
	ZrotationDegrees = 45.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.2f, 2.4f, 3.35f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Assign texture and UV scaling
	SetShaderTexture("silverthread");
	SetTextureUVScale(5.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/


	/*** box renders	***/

	// mic stand 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.7f, 2.4f, .1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.1f, 1.0f, 1.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Assign texture and UV scaling
	SetShaderTexture("blackmetal");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("matte");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	// mic stand to mic body connection
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.7f, .4f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.1f, 2.0f, 2.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Assign texture and UV scaling
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("matte");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	/*** torus renders microphone	***/

	
	// top mic ring
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.9f, .9f, .4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 45.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 1.5f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	
	//Assign texture and UV scaling
	SetShaderTexture("silverring");
	SetTextureUVScale(6.0, 2.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/


	// bottom mic ring
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.9f, .9f, .4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 2.4f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Assign texture and UV scaling
	SetShaderTexture("silverring");
	SetTextureUVScale(6.0, 2.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/
}


/*************************************************
* RenderClayBall()
*
* This method renders the shape for the clay ball
* ************************************************/
void SceneManager::RenderClayBall()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.5f, .5f, .5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(.5f, 0.5f, 6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Assign texture and UV scaling
	SetShaderTexture("pink");
	SetTextureUVScale(1.0f, 1.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/
}

/*************************************************
* RenderEnergyDrink()
*
* This method renders the shape for the canned drink
* ************************************************/
void SceneManager::RenderEnergyDrink()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.6f, 3.2f, .6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 0.0f, 6.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Assign texture and UV scaling
	SetShaderColor(.3f, .3f, .3f, 1.0f);
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
}


/*************************************************
* RenderHeatGun()
*
* This method renders the shapes for the Heat Gun
* ************************************************/
void SceneManager::RenderHeatGun()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	// Battery / base of Heat Gun
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 1.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 0.5f, 6.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Assign texture and UV scaling
	SetShaderColor(.15f, .15f, .15f, 1.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// Heat Gun motor assembly
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 1.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 0.5f, 2.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Assign texture and UV scaling
	SetShaderColor(.3f, .4f, .1f, 1.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	// Heat Gun handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.4f, 3.1f, .4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 7.5f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 0.5f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Assign texture and UV scaling
	SetShaderColor(.3f, .4f, .15f, 1.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/


	// Heat Gun blower piece
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.5f, .75f, .5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 0.5f, 2.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Assign texture and UV scaling
	SetShaderColor(.15f, .15f, .15f, 1.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/
}