#include "mesh.h"
#include "CS248/lodepng.h"

#include <cassert>
#include <sstream>

#include "../static_scene/object.h"
#include "../static_scene/light.h"

using namespace std;
using std::ostringstream;

namespace CS248 {
namespace DynamicScene {

// For use in choose_hovered_subfeature.
static const double low_threshold = .1;
static const double mid_threshold = .2;
static const double high_threshold = 1.0 - low_threshold;

Mesh::Mesh(Collada::PolymeshInfo &polyMesh, const Matrix4x4 &transform, const std::string shader_prefix) {
    // Build halfedge mesh from polygon soup
    for (const Collada::Polygon &p : polyMesh.polygons) {
        polygons.push_back(p.vertex_indices);
    }

    vector<Vector3D> vertices = polyMesh.vertices;  // DELIBERATE COPY.
    simple_renderable = polyMesh.is_obj_file;
    simple_colors = polyMesh.is_mtl_file;
    if (!simple_renderable) return;
	polygons_carbon_copy = polyMesh.polygons;
	position = polyMesh.position;
	rotation = polyMesh.rotation;
	scale = polyMesh.scale;
	do_texture_mapping = false;
	do_normal_mapping = false;
	do_environment_mapping = false;
	do_blending = false;
	do_disney_brdf = false;

	this->vertices.reserve(polyMesh.vertices.size());
	this->normals.reserve(polyMesh.normals.size());
	this->texture_coordinates.reserve(polyMesh.texcoords.size());
	for(int i = 0; i < polyMesh.vertices.size(); ++i) {
		Vector3D &u = polyMesh.vertices[i];
		Vector3Df v;
		v.x = u.x;
		v.y = u.y;
		v.z = u.z;
		this->vertices.push_back(v);
	}
	for(int i = 0; i < polyMesh.normals.size(); ++i) {
		Vector3D &u = polyMesh.normals[i];
		Vector3Df v;
		v.x = u.x;
		v.y = u.y;
		v.z = u.z;
		this->normals.push_back(v);
	}
	for(int i = 0; i < polyMesh.texcoords.size(); ++i) {
		Vector2D &u = polyMesh.texcoords[i];
		Vector2Df v;
		v.x = u.x;
		v.y = u.y;
		this->texture_coordinates.push_back(v);
	}
	diffuse_colors.reserve(polygons.size());
	for(int i = 0; i < polygons.size(); ++i) {
		Vector3D &u = polyMesh.material_diffuse_parameters[i];
		Vector3Df v;
		v.x = u.x;
		v.y = u.y;
		v.z = u.z;
		diffuse_colors.push_back(v);
	}

	vertexData.reserve(polygons.size() * 3);
	diffuse_colorData.reserve(polygons.size() * 3);
	normalData.reserve(polygons.size() * 3);
	texcoordData.reserve(polygons.size() * 3);
	tangentData.reserve(polygons.size() * 3);

	for(int i = 0; i < polygons.size(); ++i) {
		for(int j = 0; j < 3; ++j) {
  			vertexData.push_back(this->vertices[polyMesh.polygons[i].vertex_indices[j]]);
			diffuse_colorData.push_back(this->diffuse_colors[i]);
  			normalData.push_back(this->normals[polyMesh.polygons[i].normal_indices[j]]);
		}
		if(this->texture_coordinates.size() > 0) {
			for(int j = 0; j < 3; ++j) {
				texcoordData.push_back(this->texture_coordinates[polyMesh.polygons[i].texcoord_indices[j]]);
			}
		}
	}

	for(int i = 0; i < vertexData.size(); i+=3) {
		Vector3Df v0 = vertexData[i+0];
		Vector3Df v1 = vertexData[i+1];
		Vector3Df v2 = vertexData[i+2];

		Vector2Df uv0 = texcoordData[i+0];
		Vector2Df uv1 = texcoordData[i+1];
		Vector2Df uv2 = texcoordData[i+2];

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

		this->tangentData.push_back(tangent);
		this->tangentData.push_back(tangent);
		this->tangentData.push_back(tangent);
	}

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3Df) * vertexData.size(), (void*)&vertexData[0], GL_STATIC_DRAW);

	glGenBuffers(1, &normalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3Df) * normalData.size(), (void*)&normalData[0], GL_STATIC_DRAW);
	  
	glGenBuffers(1, &texcoordBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, texcoordBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vector2Df) * texcoordData.size(), (void*)&texcoordData[0], GL_STATIC_DRAW);

	glGenBuffers(1, &tangentBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, tangentBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3Df) * tangentData.size(), (void*)&tangentData[0], GL_STATIC_DRAW);

	if(diffuse_colorData.size() > 0) {
		glGenBuffers(1, &diffuse_colorBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, diffuse_colorBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3Df) * diffuse_colorData.size(), (void*)&diffuse_colorData[0], GL_STATIC_DRAW);
	}

	glBindVertexArray(0);

	if(polyMesh.vert_filename != "" && polyMesh.frag_filename != "")
		shaders.push_back(Shader(polyMesh.vert_filename, polyMesh.frag_filename, shader_prefix, shader_prefix));

	uniform_strings = polyMesh.uniform_strings;
	uniform_values = polyMesh.uniform_values;

    if(uniform_strings.size() == 0 && polyMesh.frag_filename.find("disney.frag") != std::string::npos) {
        uniform_strings.push_back("metallic");
        uniform_values.push_back(0);

        uniform_strings.push_back("subsurface");
        uniform_values.push_back(1);

        uniform_strings.push_back("specular");
        uniform_values.push_back(0);

        uniform_strings.push_back("roughness");
        uniform_values.push_back(0);

        uniform_strings.push_back("specularTint");
        uniform_values.push_back(0);

        uniform_strings.push_back("anisotropic");
        uniform_values.push_back(0);

        uniform_strings.push_back("sheen");
        uniform_values.push_back(0);

        uniform_strings.push_back("sheenTint");
        uniform_values.push_back(0);

        uniform_strings.push_back("clearcoat");
        uniform_values.push_back(0);

        uniform_strings.push_back("clearcoatGloss");
        uniform_values.push_back(0);
    }

	if(simple_colors) return;
	
    do_disney_brdf = polyMesh.is_disney;

	if(polyMesh.diffuse_filename != "") {
		unsigned int error = lodepng::decode(diffuse_texture, diffuse_texture_width, diffuse_texture_height, polyMesh.diffuse_filename);
		if(error) cerr << "Texture (diffuse) loading error = " << polyMesh.diffuse_filename << endl;
		glGenTextures(1, &diffuseId);
		glBindTexture(GL_TEXTURE_2D, diffuseId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, diffuse_texture_width, diffuse_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void *)&diffuse_texture[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    do_texture_mapping = true;
    } else do_texture_mapping = false;
	if(polyMesh.normal_filename != "") {
		unsigned int error = lodepng::decode(normal_texture, normal_texture_width, normal_texture_height, polyMesh.normal_filename);
		if(error) cerr << "Texture (normal) loading error = " << polyMesh.normal_filename << endl;
		glGenTextures(1, &normalId);
		glBindTexture(GL_TEXTURE_2D, normalId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, normal_texture_width, normal_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void *)&normal_texture[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    do_normal_mapping = true;
    } else do_normal_mapping = false;
	if(polyMesh.environment_filename != "") {
		unsigned int error = lodepng::decode(environment_texture, environment_texture_width, environment_texture_height, polyMesh.environment_filename);
		if(error) cerr << "Texture (environment) loading error = " << polyMesh.environment_filename << endl;
		glGenTextures(1, &environmentId);
		glBindTexture(GL_TEXTURE_2D, environmentId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, environment_texture_width, environment_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void *)&environment_texture[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    do_environment_mapping = true;
    } else do_environment_mapping = false;
	if(polyMesh.alpha_filename != "") {
		unsigned int error = lodepng::decode(alpha_texture, alpha_texture_width, alpha_texture_height, polyMesh.alpha_filename);
		if(error) cerr << "Texture (alpha) loading error = " << polyMesh.alpha_filename << endl;
		glGenTextures(1, &alphaId);
		glBindTexture(GL_TEXTURE_2D, alphaId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, alpha_texture_width, alpha_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void *)&alpha_texture[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    do_blending = true;
    } else do_blending = false;
	if(polyMesh.stub1_filename != "") {
		unsigned int error = lodepng::decode(stub1_texture, stub1_texture_width, stub1_texture_height, polyMesh.stub1_filename);
		if(error) cerr << "Texture (stub1) loading error = " << polyMesh.stub1_filename << endl;
		glGenTextures(1, &stub1Id);
		glBindTexture(GL_TEXTURE_2D, stub1Id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, stub1_texture_width, stub1_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void *)&stub1_texture[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	if(polyMesh.stub2_filename != "") {
		unsigned int error = lodepng::decode(stub2_texture, stub2_texture_width, stub2_texture_height, polyMesh.stub2_filename);
		if(error) cerr << "Texture (stub2) loading error = " << polyMesh.stub2_filename << endl;
		glGenTextures(1, &stub2Id);
		glBindTexture(GL_TEXTURE_2D, stub2Id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, stub2_texture_width, stub2_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void *)&stub2_texture[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	if(polyMesh.stub3_filename != "") {
		unsigned int error = lodepng::decode(stub3_texture, stub3_texture_width, stub3_texture_height, polyMesh.stub3_filename);
		if(error) cerr << "Texture (stub3) loading error = " << polyMesh.stub3_filename << endl;
		glGenTextures(1, &stub3Id);
		glBindTexture(GL_TEXTURE_2D, stub3Id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, stub3_texture_width, stub3_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void *)&stub3_texture[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

Mesh::~Mesh() {
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &normalBuffer);
    glDeleteBuffers(1, &texcoordBuffer);
	glDeleteBuffers(1, &tangentBuffer);

    if(diffuse_colorData.size() > 0) glDeleteBuffers(1, &diffuse_colorBuffer);
}

void Mesh::draw_pretty() {
  glPushMatrix();
  glTranslatef(position.x, position.y, position.z);
  glRotatef(rotation.x, 1.0f, 0.0f, 0.0f);
  glRotatef(rotation.y, 0.0f, 1.0f, 0.0f);
  glRotatef(rotation.z, 0.0f, 0.0f, 1.0f);
  glScalef(scale.x, scale.y, scale.z);

  glBindTexture(GL_TEXTURE_2D, 0);
  Spectrum white = Spectrum(1., 1., 1.);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, &white.r);

  // Enable lighting for faces
  glEnable(GL_LIGHTING);
  glDisable(GL_BLEND);
  draw_faces(true);

  glPopMatrix();
}

void Mesh::draw() {
  glPushMatrix();

  glTranslatef(position.x, position.y, position.z);
  glRotatef(rotation.x, 1.0f, 0.0f, 0.0f);
  glRotatef(rotation.y, 0.0f, 1.0f, 0.0f);
  glRotatef(rotation.z, 0.0f, 0.0f, 1.0f);
  glScalef(scale.x, scale.y, scale.z);

  float deg2Rad = M_PI / 180.0;
  
  Matrix4x4 T = Matrix4x4::translation(position);
  Matrix4x4 RX = Matrix4x4::rotation(rotation.x * deg2Rad, Matrix4x4::Axis::X);
  Matrix4x4 RY = Matrix4x4::rotation(rotation.y * deg2Rad, Matrix4x4::Axis::Y);
  Matrix4x4 RZ = Matrix4x4::rotation(rotation.z * deg2Rad, Matrix4x4::Axis::Z);
  Matrix4x4 scaleXform = Matrix4x4::scaling(scale);
  
  Matrix4x4 xform = T * RX * RY * RZ * scaleXform;

  // inv transpose for transforming normals
  Matrix4x4 xformNorm = (RX * RY * RZ * scaleXform).inv().T();    

  // copy to column-major buffers for hand off to OpenGL  
  int idx = 0;
  for (int i=0; i<4; i++) {
      const Vector4D& c = xform.column(i); 
      glObj2World[idx++] = c[0]; glObj2World[idx++] = c[1]; glObj2World[idx++] = c[2]; glObj2World[idx++] = c[3];
  }

  idx = 0;
  for (int i=0; i<3; i++) {
      const Vector4D& c = xformNorm.column(i); 
      glObj2WorldNorm[idx++] = c[0]; glObj2WorldNorm[idx++] = c[1]; glObj2WorldNorm[idx++] = c[2];
  }  
  
  glDisable(GL_BLEND);
  glEnable(GL_LIGHTING);
  draw_faces(false);

  glPopMatrix();

}

void Mesh::draw_faces(bool smooth) const {
    if(!simple_renderable) return;

    for(int i = 0; i < shaders.size(); ++i) {
        GLuint programID = shaders[i]._programID;
      
        glUseProgram(programID);
	    glBindVertexArray(vao);

        for(int j = 0; j < scene->patterns.size(); ++j) {
            DynamicScene::PatternObject &po = scene->patterns[j];
            int uniformLocation  = glGetUniformLocation(programID, po.name.c_str());
            if(uniformLocation >= 0) {
                if(po.type == 0) {
                    glUniform3f(uniformLocation, po.v.x, po.v.y, po.v.z);
                } else if(po.type == 1) {
                    glUniform1f(uniformLocation, po.s);
                }
            }
        }

		for(int j = 0; j < uniform_strings.size(); ++j) {
			int uniformLocation = glGetUniformLocation(programID, uniform_strings[j].c_str());
			if(uniformLocation >= 0) {
				glUniform1f(uniformLocation, uniform_values[j]);
			}
		}

        int uniformLocation = glGetUniformLocation(programID, "useTextureMapping");
        if(uniformLocation >= 0)
            glUniform1i(uniformLocation, do_texture_mapping ? 1 : 0);

        uniformLocation = glGetUniformLocation(programID, "useNormalMapping");
        if(uniformLocation >= 0)
            glUniform1i(uniformLocation, do_normal_mapping ? 1 : 0);

        uniformLocation = glGetUniformLocation(programID, "useEnvironmentMapping");
        if(uniformLocation >= 0)
            glUniform1i(uniformLocation, do_environment_mapping ? 1 : 0);
 
        uniformLocation = glGetUniformLocation(programID, "useBlending");
        if(uniformLocation >= 0)
            glUniform1i(uniformLocation, do_blending ? 1 : 0);
       
        uniformLocation = glGetUniformLocation(programID, "useDisneyBRDF");
        if(uniformLocation >= 0)
            glUniform1i(uniformLocation, do_disney_brdf ? 1 : 0);
       
        uniformLocation = glGetUniformLocation(programID, "obj2world");
        if(uniformLocation >= 0) {
            glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glObj2World);
        }
        
        uniformLocation = glGetUniformLocation(programID, "obj2worldNorm");
        if(uniformLocation >= 0) {
            glUniformMatrix3fv(uniformLocation, 1, GL_FALSE, glObj2WorldNorm);
        }
        
        int diffuseTextureID  = glGetUniformLocation(programID, "diffuseTextureSampler");
        if(diffuseTextureID >= 0) {
	        glActiveTexture(GL_TEXTURE0);
	        glBindTexture(GL_TEXTURE_2D, diffuseId);
            glUniform1i(diffuseTextureID, 0);
        }

        int normalTextureID  = glGetUniformLocation(programID, "normalTextureSampler");
        if(normalTextureID >= 0) {
	        glActiveTexture(GL_TEXTURE1);
	        glBindTexture(GL_TEXTURE_2D, normalId);
            glUniform1i(normalTextureID, 1);
        }

        int environmentTextureID  = glGetUniformLocation(programID, "environmentTextureSampler");
        if(environmentTextureID >= 0) {
	        glActiveTexture(GL_TEXTURE2);
	        glBindTexture(GL_TEXTURE_2D, environmentId);
            glUniform1i(environmentTextureID, 2);
        }

        int alphaTextureID  = glGetUniformLocation(programID, "blendTextureSampler");
        if(alphaTextureID >= 0) {
	        glActiveTexture(GL_TEXTURE3);
	        glBindTexture(GL_TEXTURE_2D, alphaId);
            glUniform1i(alphaTextureID, 3);
        }

        int stub1TextureID  = glGetUniformLocation(programID, "stub1TextureSampler");
        if(stub1TextureID >= 0) {
	        glActiveTexture(GL_TEXTURE4);
	        glBindTexture(GL_TEXTURE_2D, stub1Id);
            glUniform1i(stub1TextureID, 4);
        }

        int stub2TextureID  = glGetUniformLocation(programID, "stub2TextureSampler");
        if(stub2TextureID >= 0) {
	        glActiveTexture(GL_TEXTURE5);
	        glBindTexture(GL_TEXTURE_2D, stub2Id);
            glUniform1i(stub2TextureID, 5);
        }

        int stub3TextureID  = glGetUniformLocation(programID, "stub3TextureSampler");
        if(stub3TextureID >= 0) {
	        glActiveTexture(GL_TEXTURE6);
	        glBindTexture(GL_TEXTURE_2D, stub3Id);
            glUniform1i(stub2TextureID, 6);
        }

        Vector3D camPosition = scene->camera->position();
        float v1 = camPosition.x;
        float v2 = camPosition.y;
        float v3 = camPosition.z;
        uniformLocation = glGetUniformLocation( programID, "camera_position" );
        if(uniformLocation >=0) glUniform3f( uniformLocation, v1, v2, v3 );

        uniformLocation = glGetUniformLocation( programID, "num_directional_lights" );
        if(uniformLocation >= 0) glUniform1i( uniformLocation, scene->directional_lights.size() );

        for(int j = 0; j < scene->directional_lights.size(); ++j) {
            StaticScene::DirectionalLight *light = scene->directional_lights[j];
            float v1 = light->dirToLight.x;
            float v2 = light->dirToLight.y;
            float v3 = light->dirToLight.z;		
            string str = "directional_light_vectors[" + std::to_string(j) + "]";
            uniformLocation = glGetUniformLocation( programID, str.c_str() );
            if(uniformLocation >= 0) glUniform3f( uniformLocation, v1, v2, v3 );
        }

        uniformLocation = glGetUniformLocation( programID, "num_point_lights" );
        if(uniformLocation >= 0) glUniform1i( uniformLocation, scene->point_lights.size() );

        for(int j = 0; j < scene->point_lights.size(); ++j) {
            StaticScene::PointLight *light = scene->point_lights[j];
            float v1 = light->position.x;
            float v2 = light->position.y;
            float v3 = light->position.z;
            string str = "point_light_positions[" + std::to_string(j) + "]";
            uniformLocation = glGetUniformLocation( programID, str.c_str() );
            if(uniformLocation >= 0) glUniform3f( uniformLocation, v1, v2, v3 );
        }

	    int vert_loc = glGetAttribLocation(programID, "vtx_position");
	    if (vert_loc >= 0) {
            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
            glVertexAttribPointer(vert_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(vert_loc);
	    }

	    int dclr_loc = glGetAttribLocation(programID, "vtx_diffuse_color");
	    if (dclr_loc >= 0) {
            glBindBuffer(GL_ARRAY_BUFFER, diffuse_colorBuffer);
            glVertexAttribPointer(dclr_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(dclr_loc);
	    }

	    int normal_loc = glGetAttribLocation(programID, "vtx_normal");
        if(normal_loc >= 0) {
            glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
            glVertexAttribPointer(normal_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(normal_loc);
        }

	    int tex_loc = glGetAttribLocation(programID, "vtx_texcoord");
	    if (tex_loc >= 0) {
            glBindBuffer(GL_ARRAY_BUFFER, texcoordBuffer);
            glVertexAttribPointer(tex_loc, 2, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(tex_loc);
	    }

        int tan_loc = glGetAttribLocation(programID, "vtx_tangent");
        if (tan_loc >= 0) {
            glBindBuffer(GL_ARRAY_BUFFER, tangentBuffer);
            glVertexAttribPointer(tan_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(tan_loc);
        }

	    glDrawArrays(GL_TRIANGLES, 0, polygons.size() * 3);

	    glBindVertexArray(0);
        glUseProgram(0);
	    glBindTexture(GL_TEXTURE_2D, 0);
    }
}

BBox Mesh::get_bbox() {
  BBox bbox;
  if (simple_renderable) {
	  for(int i = 0; i < vertices.size(); ++i) {
		Vector3Df &u = vertices[i];
		Vector3D v(u.x, u.y, u.z);
		bbox.expand(v);
	  }
	  return bbox;
  }
  return bbox;
}

StaticScene::SceneObject *Mesh::get_static_object() {
  return nullptr;
//  return new StaticScene::Mesh(mesh);
}

Matrix3x3 rotateMatrix(float ux, float uy, float uz, float theta) {
  Matrix3x3 out = Matrix3x3();
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

StaticScene::SceneObject *Mesh::get_transformed_static_object(double t) {
  return nullptr;
}

}  // namespace DynamicScene
}  // namespace CS248
