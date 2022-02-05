#include "mesh.h"
#include "CS248/lodepng.h"

#include <cassert>
#include <sstream>

#include "../static_scene/object.h"
#include "../static_scene/light.h"

#include "../application.h"
#include "../gl_utils.h"

using namespace std;
using std::ostringstream;

namespace CS248 {
namespace DynamicScene {


Mesh::Mesh(Collada::PolymeshInfo& polyMesh, const Matrix4x4& transform) {

	checkGLError("begin mesh constructor");
    
	doTextureMapping_ = false;
	doNormalMapping_ = false;
	doEnvironmentMapping_ = false;
	useMirrorBrdf_ = polyMesh.is_mirror_brdf;
	phongSpecExponent_ = polyMesh.phong_spec_exp;

    //printf("Mesh details:\n");
    //printf("   num polys:     %lu\n", polyMesh.polygons.size());
    //printf("   num verts:     %lu\n", polyMesh.vertices.size());
    //printf("   num normals:   %lu\n", polyMesh.normals.size());
    //printf("   num texcoords: %lu\n", polyMesh.texcoords.size());
 
	position_ = polyMesh.position;
	rotation_ = polyMesh.rotation;
	scale_ = polyMesh.scale;

    // HACK(kayvonf): move the transform (parsed from the scene json and passed to the constructor by the caller)
    // into the object's position. There's definitely a cleaner way to do this since position must have also been
    // parsed out of the json by the collada parser.
    position_ = Vector3D(transform[3][0], transform[3][1], transform[3][2]);
    scale_ = Vector3D(transform[0][0], transform[1][1], transform[2][2]);


    // FIXME(kayvonf): I do not understand why we are copying data from the PolyMesh structure
    // into local variables below (step 1), and then copying the results into the non-indexed
    // buffers (step 2). Step 2 could be completed directly from the PolyMesh structure.
   
    // Step 1: 
    //
    // Copy data from polyMesh structure to local variables
    //

    numTriangles_ = polyMesh.polygons.size();

    vector<Vector3Df> positions;
    vector<Vector3Df> normals;
    vector<Vector2Df> textureCoordinates;
    vector<Vector3Df> diffuseColors;  // (this is per face data)

	positions.reserve(polyMesh.vertices.size());
	normals.reserve(polyMesh.normals.size());
	textureCoordinates.reserve(polyMesh.texcoords.size());
	diffuseColors.reserve(polyMesh.polygons.size()); // one per face?

	for (int i = 0; i < polyMesh.vertices.size(); ++i) {
		Vector3Df v;
		v.x = polyMesh.vertices[i].x;
		v.y = polyMesh.vertices[i].y;
		v.z = polyMesh.vertices[i].z;
		positions.push_back(v);
	}
	for (int i = 0; i < polyMesh.normals.size(); ++i) {
		Vector3Df v;
		v.x = polyMesh.normals[i].x;
		v.y = polyMesh.normals[i].y;
		v.z = polyMesh.normals[i].z;
		normals.push_back(v);
	}
	for (int i = 0; i < polyMesh.texcoords.size(); ++i) {
		Vector2Df v;
		v.x = polyMesh.texcoords[i].x;
		v.y = polyMesh.texcoords[i].y;
		textureCoordinates.push_back(v);
	}
	for (int i = 0; i < polyMesh.polygons.size(); ++i) {
		Vector3Df v;
		v.x = polyMesh.material_diffuse_parameters[i].x;
		v.y = polyMesh.material_diffuse_parameters[i].y;
		v.z = polyMesh.material_diffuse_parameters[i].z;
		diffuseColors.push_back(v);
	}

	//
	// Step 2:
	// 
    // These are the buffers that will be handed to glVertexArray calls.
    // Allocate and populate them here.  They are a non-indexed representation, in that there are
    // three values per polygon.

	positionData_.reserve(3 * numTriangles_);
	normalData_.reserve(3 * numTriangles_);
	diffuseColorData_.reserve(3 * numTriangles_);
	texcoordData_.reserve(3 * numTriangles_);
	tangentData_.reserve(3 * numTriangles_);

    // populate buffers for vertex position, normal, diffuse color, and texcoord
	for (int i = 0; i < numTriangles_; ++i) {
		for (int j = 0; j < 3; ++j) {
  			positionData_.push_back(positions[polyMesh.polygons[i].vertex_indices[j]]);
            normalData_.push_back(normals[polyMesh.polygons[i].normal_indices[j]]);
	        diffuseColorData_.push_back(diffuseColors[i]);
		}
		if (textureCoordinates.size() > 0) {
			for (int j = 0; j < 3; ++j) {
				texcoordData_.push_back(textureCoordinates[polyMesh.polygons[i].texcoord_indices[j]]);
			}
		}
	}

	// compute tangents: this is a loop over triangles (not verts)
	for (int i=0; i < positionData_.size(); i+=3) {
		Vector3Df v0 = positionData_[i+0];
		Vector3Df v1 = positionData_[i+1];
		Vector3Df v2 = positionData_[i+2];

		Vector2Df uv0 = texcoordData_[i+0];
		Vector2Df uv1 = texcoordData_[i+1];
		Vector2Df uv2 = texcoordData_[i+2];

		Vector3Df deltaPos1;
		deltaPos1.x = v1.x-v0.x;
		deltaPos1.y = v1.y-v0.y;
		deltaPos1.z = v1.z-v0.z;

		Vector3Df deltaPos2;
		deltaPos2.x = v2.x-v0.x;
		deltaPos2.y = v2.y-v0.y;
		deltaPos2.z = v2.z-v0.z;    

		Vector2Df deltaUV1;
		deltaUV1.x = uv1.x - uv0.x;
		deltaUV1.y = uv1.y - uv0.y;

		Vector2Df deltaUV2;
		deltaUV2.x = uv2.x - uv0.x;
		deltaUV2.y = uv2.y - uv0.y;

		float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

		Vector3Df tangent;
		tangent.x = (deltaPos1.x * deltaUV2.y - deltaPos2.x * deltaUV1.y)*r;
		tangent.y = (deltaPos1.y * deltaUV2.y - deltaPos2.y * deltaUV1.y)*r;
		tangent.z = (deltaPos1.z * deltaUV2.y - deltaPos2.z * deltaUV1.y)*r;

		// all three verts in a triangle share the same tangent
		tangentData_.push_back(tangent);
		tangentData_.push_back(tangent);
		tangentData_.push_back(tangent);
	}

	// Allocate resources in GL
	gl_mgr_ = GLResourceManager::instance();

	// GL vertex array object
	vertexArrayId_ = gl_mgr_->createVertexArray();

	//
	// allocate all the OpenGL vertex buffers, and copy the contents of the local buffers into them 
	//

	// Sanity check for struct layout in case of unconventional compiler
	static_assert(sizeof(Vector3Df) == 3*sizeof(float), "Fatal error: Vector3Df struct has extra padding on this platform.");
	static_assert(sizeof(Vector2Df) == 2*sizeof(float), "Fatal error: Vector2Df struct has extra padding on this platform.");

	checkGLError("begin mesh vertex buffer setup");

	positionBufferId_ = gl_mgr_->createVertexBufferFromData((const float *)&positionData_[0], positionData_.size()*3);

	// printf("Position buffer object: %u\n", positionBufferId.id);

	checkGLError("before normal buffers");

	normalBufferId_ = gl_mgr_->createVertexBufferFromData((const float *)&normalData_[0], normalData_.size()*3);

	// printf("Normal buffer object: %u\n", normalBufferId.id);

	checkGLError("before texcoord buffers");

	texcoordBufferId_ = gl_mgr_->createVertexBufferFromData((const float*)&texcoordData_[0], texcoordData_.size()*2);
	  
	// printf("Texcoord buffer object: %u\n", texcoordBufferId.id);

	checkGLError("before tangent buffers");

	tangentBufferId_ = gl_mgr_->createVertexBufferFromData((const float*)&tangentData_[0], tangentData_.size()*3);

	// printf("Tangent buffer object: %u\n", tangentBufferId.id);

	checkGLError("before diffuse color buffers");

	diffuseColorBufferId_ = gl_mgr_->createVertexBufferFromData((const float*)&diffuseColorData_[0], diffuseColorData_.size()*3);

	// printf("Diffuse color buffer object: %u\n", diffuseColorBufferId.id);

	//
	// allocate all the textures
	//

    // Texture data
    vector<unsigned char> diffuse_texture;
    vector<unsigned char> normal_texture;
    vector<unsigned char> environment_texture;
    unsigned int diffuse_texture_width, diffuse_texture_height;
    unsigned int normal_texture_width, normal_texture_height;
    unsigned int environment_texture_width, environment_texture_height;

    // create the diffuse albedo texture map
	if (polyMesh.diffuse_filename != "") {
		unsigned int error = lodepng::decode(diffuse_texture, diffuse_texture_width, diffuse_texture_height, polyMesh.diffuse_filename);
		if(error) cerr << "Texture (diffuse) loading error = " << polyMesh.diffuse_filename << endl;
		diffuseTextureId_ = gl_mgr_->createTextureFromData(diffuse_texture.data(), diffuse_texture_width, diffuse_texture_height);
	    doTextureMapping_ = true;
    } else {
        doTextureMapping_ = false;
    }

    // create the normal map texture map
    if (polyMesh.normal_filename != "") {
		unsigned int error = lodepng::decode(normal_texture, normal_texture_width, normal_texture_height, polyMesh.normal_filename);
		if(error) cerr << "Texture (normal) loading error = " << polyMesh.normal_filename << endl;
		normalTextureId_ = gl_mgr_->createTextureFromData(normal_texture.data(), normal_texture_width, normal_texture_height);
	    doNormalMapping_ = true;
    } else {
        doNormalMapping_ = false;
    }

    // create the environment lighting texture map
    if (polyMesh.environment_filename != "") {
		unsigned int error = lodepng::decode(environment_texture, environment_texture_width, environment_texture_height, polyMesh.environment_filename);
		if(error) cerr << "Texture (environment) loading error = " << polyMesh.environment_filename << endl;
		environmentTextureId_ = gl_mgr_->createTextureFromData(environment_texture.data(), environment_texture_width, environment_texture_height);
	    doEnvironmentMapping_ = true;
    } else {
        doEnvironmentMapping_ = false;
    }

	//
	// allocate the shader for this mesh
	//

	checkGLError("before mesh create shader");

	shader_ = new Shader(polyMesh.vert_filename, polyMesh.frag_filename);

	checkGLError("done mesh create shader");
}

Mesh::~Mesh() {

	gl_mgr_->freeVertexArray(vertexArrayId_);
	gl_mgr_->freeVertexBuffer(positionBufferId_);
	gl_mgr_->freeVertexBuffer(normalBufferId_);
	gl_mgr_->freeVertexBuffer(diffuseColorBufferId_);
	gl_mgr_->freeVertexBuffer(texcoordBufferId_);
	gl_mgr_->freeVertexBuffer(tangentBufferId_);

	if (doTextureMapping_) {
		gl_mgr_->freeTexture(diffuseTextureId_);
	}
	if (doNormalMapping_) {
		gl_mgr_->freeTexture(normalTextureId_);
	}
	if (doEnvironmentMapping_) {
		gl_mgr_->freeTexture(environmentTextureId_);
	}

    delete shader_;
}

/*
 * Draw the mesh
 */
void Mesh::draw(const Matrix4x4& worldToNDC) const {
	internalDraw(false, worldToNDC);
}

/*
 * Draw the mesh as part of a shadow map generation rendering pass
 */
void Mesh::drawShadow(const Matrix4x4& worldToNDC) const {
	internalDraw(true, worldToNDC);
}

void Mesh::internalDraw(bool shadowPass, const Matrix4x4& worldToNDC) const {

	// printf("Top of Mesh::internalDraw  (%lu shadowed lights)\n", scene->getNumShadowedLights());

	checkGLError("begin draw faces");
    
	Matrix4x4 objectToWorld = getObjectToWorld();
	Matrix3x3 objectToWorldForNormals = getObjectToWorldForNormals();
	Matrix4x4 mvp = worldToNDC * objectToWorld;

	// cout << "obj2world: " << objectToWorld << endl;

	auto vertex_array_bind = gl_mgr_->bindVertexArray(vertexArrayId_);

    if (shadowPass) {

    	Shader* shadowShader = scene_->getShadowShader();

    	auto shader_bind = shadowShader->bind();
        shadowShader->setMatrixParameter("mvp", mvp);
        shadowShader->setVertexBuffer("vtx_position", 3, positionBufferId_);
		shadowShader->setVertexBuffer("vtx_normal", 3, normalBufferId_);
        shadowShader->setMatrixParameter("obj2worldNorm", objectToWorldForNormals);

		checkGLError("before glDrawArrays in shadow pass");
        glDrawArrays(GL_TRIANGLES, 0, 3 * numTriangles_);

    } else {

    	checkGLError("before use program");

    	auto shader_bind = shader_->bind();

		checkGLError("before bind uniforms");

    	shader_->setScalarParameter("useTextureMapping", doTextureMapping_ ? 1 : 0);
    	shader_->setScalarParameter("useNormalMapping", doNormalMapping_ ? 1 : 0);        
        shader_->setScalarParameter("useEnvironmentMapping", doEnvironmentMapping_ ? 1 : 0);
        shader_->setScalarParameter("useMirrorBRDF", useMirrorBrdf_ ? 1 : 0);
        shader_->setScalarParameter("spec_exp", phongSpecExponent_);

		checkGLError("after binding the scalars");

        shader_->setVectorParameter("camera_position", scene_->getCamera()->getPosition());

		checkGLError("after binding camera position");

        shader_->setMatrixParameter("obj2world", objectToWorld);

		checkGLError("after binding o2w");

        shader_->setMatrixParameter("obj2worldNorm", objectToWorldForNormals);

		checkGLError("after binding everything but mvp");

        shader_->setMatrixParameter("mvp", mvp);

		checkGLError("after binding mvp");

        int numShadowedLights = scene_->getNumShadowedLights();

        // TODO CS248 Part 5.2: Shadow Mapping
        // You need to pass an array of matrices to the shader.
        // They should go from object space to the "light space" for each spot light.
        // In this way, the shader can compute the texture coordinate to sample from the
        // Shadow Map given any point on the object.
        // For examples of passing arrays to the shader, look below for "directional_light_vectors[]" etc.


		checkGLError("after bind uniforms, about to bind textures");

        // bind texture samplers ///////////////////////////////////

        if (doTextureMapping_)
        	shader_->setTextureSampler("diffuseTextureSampler", diffuseTextureId_);

        // TODO CS248 Part 3: Normal Mapping:
        // You want to pass the normal texture into the shader program.
        // See diffuseTextureSampler for an example of passing textures.

        // TODO CS248 Part 4: Environment Mapping:
        // You want to pass the environment texture into the shader program.
        // See diffuseTextureSampler for an example of passing textures.

        // TODO CS248 Part 5.2: Shadow Mapping:
        // You want to pass the array of shadow textures computed during shadow pass into the shader program.
        // See Scene::visualizeShadowMap for an example of passing texture arrays.
        // See shadow_viz.frag for an example of using texture arrays in the shader.


        // bind light parameters //////////////////////////////////

    	shader_->setScalarParameter("num_directional_lights", (int)scene_->getNumDirectionalLights());
        shader_->setScalarParameter("num_point_lights", (int)scene_->getNumPointLights());
        shader_->setScalarParameter("num_spot_lights", (int)scene_->getNumSpotLights());

        for (int j=0; j<scene_->getNumDirectionalLights(); j++) {
            string varname = "directional_light_vectors[" + std::to_string(j) + "]";
            const StaticScene::DirectionalLight* light = scene_->getDirectionalLight(j);
            shader_->setVectorParameter(varname, light->lightDir);
        }

	    checkGLError("before bind point light attributes");

        for (int j=0; j<scene_->getNumPointLights(); j++) {
            string varname = "point_light_positions[" + std::to_string(j) + "]";
            const StaticScene::PointLight* light = scene_->getPointLight(j);
            shader_->setVectorParameter(varname, light->position);
        }

	    checkGLError("before bind spotlight attributes");

        for (int j=0; j<scene_->getNumSpotLights(); j++) {

            const StaticScene::SpotLight* light = scene_->getSpotLight(j);
            string varname = "spot_light_positions[" + std::to_string(j) + "]";
            shader_->setVectorParameter(varname, light->position);

            varname = "spot_light_directions[" + std::to_string(j) + "]";
            shader_->setVectorParameter(varname, light->direction);

            varname = "spot_light_angles[" + std::to_string(j) + "]";
            shader_->setScalarParameter(varname, light->angle);

            varname = "spot_light_intensities[" + std::to_string(j) + "]";
            Vector3D value(light->radiance.r, light->radiance.g, light->radiance.b);
            shader_->setVectorParameter(varname, value);
        }

        // bind per-vertex attribute buffers.  These are "in" parameters to the vertex shader

	    checkGLError("before bind vertex attributes");

	    shader_->setVertexBuffer("vtx_position", 3, positionBufferId_);

	   	checkGLError("before bind diffuse color");

	    shader_->setVertexBuffer("vtx_diffuse_color", 3, diffuseColorBufferId_);

	   	checkGLError("before bind normal");

	    shader_->setVertexBuffer("vtx_normal", 3, normalBufferId_);

	   	checkGLError("before bind texdtcoord");

	    shader_->setVertexBuffer("vtx_texcoord", 2, texcoordBufferId_);

		checkGLError("before bind tangent");

	    shader_->setVertexBuffer("vtx_tangent", 3, tangentBufferId_);
		
		// now issue the draw command to OpenGL
		checkGLError("before glDrawArrays");
		glDrawArrays(GL_TRIANGLES, 0, 3 * numTriangles_);
	}

	checkGLError("end mesh::internalDraw");
}

void Mesh::reloadShaders() {
	shader_->reload();
}

BBox Mesh::getBBox() const {

	BBox bbox;

	// convert object-space points to world space points, and compute a world-space bounding box
	// NOTE(kayvonf): this is an inefficient implementation since we're considering each unique vertex position up to 3 times 
	for (int i=0; i<3*numTriangles_; ++i) {
		Vector4D posObj(positionData_[i].x, positionData_[i].y, positionData_[i].z, 1.f);
		Vector4D posObjWorld = getObjectToWorld() * posObj;
		bbox.expand(posObjWorld.projectTo3D());
  	}

  	return bbox;
}

/*
Matrix3x3 rotateMatrix(float ux, float uy, float uz, float theta) {
	Matrix3x3 out;
	float c = cos(theta);
	float s = sin(theta);
  	out(0, 0) = c + ux * ux * (1 - c);
  	out(0, 1) = ux * uy * (1 - c) - uz * s;
  	out(0, 2) = ux * uz * (1 - c) + uy * s;
  	out(1, 0) = uy * ux * (1 - c) + uz * s;
  	out(1, 1) = c + uy * uy * (1 - c);
  	out(1, 2) = uy * uz * (1 - c) - ux * s;
  	out(2, 0) = uz * ux * (1 - c) - uy * s;
 	out(2, 1) = uz * uy * (1 - c) + ux * s;
  	out(2, 2) = c + uz * uz * (1 - c);
  	return out;
}
*/


}  // namespace DynamicScene
}  // namespace CS248
