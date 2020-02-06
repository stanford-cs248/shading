#include "collada.h"
#include "math.h"
#include "CS248/JSON.h"

#include <assert.h>
#include <map>
#include <ctime>
#include <string>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm>

// For more verbose output, uncomment the line below.
#define stat(s)  // cerr << "[COLLADA Parser] " << s << endl;

using namespace std;

namespace CS248 {
namespace Collada {

SceneInfo* ColladaParser::scene;  // pointer to output scene description

Vector3D ColladaParser::up;                       // scene up direction
Matrix4x4 ColladaParser::transform;               // current transformation
map<string, XMLElement*> ColladaParser::sources;  // URI lookup table

// Parser Helpers //

inline Spectrum spectrum_from_string(string spectrum_string) {
  Spectrum s;

  stringstream ss(spectrum_string);
  ss >> s.r;
  ss >> s.g;
  ss >> s.b;

  return s;
}

inline Color color_from_string(string color_string) {
  Color c;

  stringstream ss(color_string);
  ss >> c.r;
  ss >> c.g;
  ss >> c.b;
  ss >> c.a;

  return c;
}

void ColladaParser::uri_load(XMLElement* xml) {
  if (xml->Attribute("id")) {
    string id = xml->Attribute("id");
    sources[id] = xml;
  }

  XMLElement* child = xml->FirstChildElement();
  while (child) {
    uri_load(child);
    child = child->NextSiblingElement();
  }
}

XMLElement* ColladaParser::uri_find(string id) {
  return (sources.find(id) != sources.end()) ? sources[id] : NULL;
}

XMLElement* ColladaParser::get_element(XMLElement* xml, string query) {
  stringstream ss(query);

  // find xml element
  XMLElement* e = xml;
  string token;
  while (e && getline(ss, token, '/')) {
    e = e->FirstChildElement(token.c_str());
  }

  // handle indirection
  if (e) {
    const char* url = e->Attribute("url");
    if (url) {
      string id = url + 1;
      e = uri_find(id);
    }
  }

  return e;
}

XMLElement* ColladaParser::get_technique_common(XMLElement* xml) {
  XMLElement* common_profile = xml->FirstChildElement("profile_COMMON");
  if (common_profile) {
    XMLElement* technique = common_profile->FirstChildElement("technique");
    while (technique) {
      string sid = technique->Attribute("sid");
      if (sid == "common") return technique;
      technique = technique->NextSiblingElement("technique");
    }
  }

  return xml->FirstChildElement("technique_common");
}

XMLElement* ColladaParser::get_technique_cmu462(XMLElement* xml) {
  XMLElement* technique = get_element(xml, "extra/technique");
  while (technique) {
    string profile = technique->Attribute("profile");
    if (profile == "CS248") return technique;
    technique = technique->NextSiblingElement("technique");
  }

  return NULL;
}

string wstring_to_string(const wstring& str)
{
    size_t sz = str.length();
    const wchar_t* p = str.c_str();
    char* tp = new char[sz + 1];
    size_t w = wcstombs(tp, p, sz);
    if (w != sz) {
        delete[] tp;
        return "";
    }
    tp[sz] = '\0';
    string ret(tp);
    delete[] tp;
    return ret;
}

int ColladaParser::load(const char* filename, SceneInfo* sceneInfo) {
  ifstream in(filename);
  if (!in.is_open()) {
    cerr << "Warning: could not open file " << filename << endl;
    return -1;
  }
  // If this file is an obj format, load it accordingly
  string filename_test(filename);

  if(filename_test.substr(filename_test.find_last_of(".") + 1) == "json"
  || filename_test.substr(filename_test.find_last_of(".") + 1) == "JSON")
  {
    stat("Loading scene JSON file...");

    string path = filename_test.substr(0, filename_test.find_last_of("/") + 1);

    stringstream buffer;
    buffer << in.rdbuf();
    string data = buffer.str();
    JSONValue *value = JSON::Parse(data.c_str());
    if(!value || !value->IsObject()) {
        cerr << "Error: bad json format" << endl;
        return -1;
    }

    JSONObject root = value->AsObject();
    scene = sceneInfo;
 
    if (root.find(L"pattern") != root.end() && root[L"pattern"]->IsArray()) {
        JSONArray pattern_json_array = root[L"pattern"]->AsArray();
		for(int i = 0; i < pattern_json_array.size(); ++i) {
			if(!pattern_json_array[i]->IsObject()) continue;
			JSONObject pattern_json_object = pattern_json_array[i]->AsObject();
			Node node = Node();
	        PatternInfo* pattern = new PatternInfo();
            pattern->pattern_type = -1;
            if (pattern_json_object.find(L"name") != pattern_json_object.end() && pattern_json_object[L"name"]->IsString()) {
                pattern->name = wstring_to_string(pattern_json_object[L"name"]->AsString());
            }
            if (pattern_json_object.find(L"display_name") != pattern_json_object.end() && pattern_json_object[L"display_name"]->IsString()) {
                pattern->display_name = wstring_to_string(pattern_json_object[L"display_name"]->AsString());
            }
            // Pattern type 0: 3D vector
            if (pattern_json_object.find(L"vector") != pattern_json_object.end() && pattern_json_object[L"vector"]->IsArray()) {
	            JSONArray v_json_array = pattern_json_object[L"vector"]->AsArray();
                if(v_json_array.size() == 3) {
                    pattern->v.x = v_json_array[0]->AsNumber();
                    pattern->v.y = v_json_array[1]->AsNumber();
                    pattern->v.z = v_json_array[2]->AsNumber();
                }
                pattern->pattern_type = 0;
            }
            // Pattern type 1: Scalar
            if (pattern_json_object.find(L"scalar") != pattern_json_object.end() && pattern_json_object[L"scalar"]->IsNumber()) {
                pattern->s = pattern_json_object[L"scalar"]->AsNumber();
                pattern->pattern_type = 1;
            }
            pattern->type = Instance::PATTERN;
            node.instance = pattern;
            scene->nodes.push_back(node);
        }
    }

    if (root.find(L"base_shader_dir") != root.end() && root[L"base_shader_dir"]->IsString()) {
      scene->base_shader_dir = path + wstring_to_string(root[L"base_shader_dir"]->AsString());
    }

    if (root.find(L"camera") != root.end() && root[L"camera"]->IsObject()) {
        JSONObject camera_json_object = root[L"camera"]->AsObject();
        Node node = Node();
        CameraInfo* camera = new CameraInfo();
        if (camera_json_object.find(L"id") != camera_json_object.end() && camera_json_object[L"id"]->IsString()) {
            camera->id = wstring_to_string(camera_json_object[L"id"]->AsString());
        }
        if (camera_json_object.find(L"name") != camera_json_object.end() && camera_json_object[L"name"]->IsString()) {
            camera->name = wstring_to_string(camera_json_object[L"name"]->AsString());
        }
        if (camera_json_object.find(L"up_axis") != camera_json_object.end() && camera_json_object[L"up_axis"]->IsString()) {
            string up_dir = wstring_to_string(camera_json_object[L"up_axis"]->AsString());
            if (up_dir == "X_UP") {
                // swap X-Y and negate Z
                transform(0, 0) = 0;
                transform(0, 1) = 1;
                transform(1, 0) = 1;
                transform(1, 1) = 0;
                transform(2, 2) = -1;

                // local up direction for lights and cameras
                up = Vector3D(1, 0, 0);

            } else if (up_dir == "Z_UP") {
                // swap Z-Y matrix and negate X
                transform(1, 1) = 0;
                transform(1, 2) = 1;
                transform(2, 1) = 1;
                transform(2, 2) = 0;
                transform(0, 0) = -1;

                // local up direction cameras
                up = Vector3D(0, 0, 1);

            } else if (up_dir == "Y_UP") {
                up = Vector3D(0, 1, 0);  // no need to correct Y_UP as its used internally
            }
            camera->up_dir = up;
        }
        if (camera_json_object.find(L"xfov") != camera_json_object.end() && camera_json_object[L"xfov"]->IsNumber()) {
            camera->hFov = camera_json_object[L"xfov"]->AsNumber();
        }
        if (camera_json_object.find(L"yfov") != camera_json_object.end() && camera_json_object[L"yfov"]->IsNumber()) {
            camera->vFov = camera_json_object[L"yfov"]->AsNumber();
        } else {
            if (camera_json_object.find(L"aspect_ratio") != camera_json_object.end() && camera_json_object[L"aspect_ratio"]->IsNumber()) {
                double aspect_ratio = camera_json_object[L"aspect_ratio"]->AsNumber();
                camera->vFov = 2 * degrees(atan(tan(radians(0.5 * camera->hFov)) / aspect_ratio));
            }
        }
        if (camera_json_object.find(L"znear") != camera_json_object.end() && camera_json_object[L"znear"]->IsNumber()) {
            camera->nClip = camera_json_object[L"znear"]->AsNumber();
        }
        if (camera_json_object.find(L"zfar") != camera_json_object.end() && camera_json_object[L"zfar"]->IsNumber()) {
            camera->fClip = camera_json_object[L"zfar"]->AsNumber();
        }
		camera->default_flag = true;
		if (camera_json_object.find(L"target_position") != camera_json_object.end() && camera_json_object[L"target_position"]->IsArray()) {
			JSONArray v_json_array = camera_json_object[L"target_position"]->AsArray();
			if(v_json_array.size() == 3) {
				camera->pos.x = v_json_array[0]->AsNumber();
				camera->pos.y = v_json_array[1]->AsNumber();
				camera->pos.z = v_json_array[2]->AsNumber();
				camera->default_flag = false;
			}
		} else {
			camera->pos = Vector3D(0, 0, 0);
		}
		if (camera_json_object.find(L"dir2cam") != camera_json_object.end() && camera_json_object[L"dir2cam"]->IsArray()) {
			JSONArray v_json_array = camera_json_object[L"dir2cam"]->AsArray();
			if(v_json_array.size() == 3) {
				camera->view_dir.x = v_json_array[0]->AsNumber();
				camera->view_dir.y = v_json_array[1]->AsNumber();
				camera->view_dir.z = v_json_array[2]->AsNumber();
				camera->default_flag = false;
			}
		} else {
			camera->view_dir = Vector3D(0, 0, -1);
		}

        camera->type = Instance::CAMERA;
        node.instance = camera;
        node.transform = Matrix4x4::identity();
        scene->nodes.push_back(node);
    }

    if (root.find(L"lights") != root.end() && root[L"lights"]->IsArray()) {
        JSONArray light_json_array = root[L"lights"]->AsArray();
		for(int i = 0; i < light_json_array.size(); ++i) {
			if(!light_json_array[i]->IsObject()) continue;
			JSONObject light_json_object = light_json_array[i]->AsObject();
			Node node = Node();
			LightInfo* light = new LightInfo();
			if (light_json_object.find(L"id") != light_json_object.end() && light_json_object[L"id"]->IsString()) {
				light->id = wstring_to_string(light_json_object[L"id"]->AsString());
			}
			if (light_json_object.find(L"name") != light_json_object.end() && light_json_object[L"name"]->IsString()) {
				light->name = wstring_to_string(light_json_object[L"name"]->AsString());
			}
			if (light_json_object.find(L"type") != light_json_object.end() && light_json_object[L"type"]->IsString()) {
				wstring light_type = light_json_object[L"type"]->AsString();
				if(light_type == L"ambient") {
					light->light_type = LightType::AMBIENT;
				} else if(light_type == L"directional") {
					light->light_type = LightType::DIRECTIONAL;
				} else if(light_type == L"area") {
					light->light_type = LightType::AREA;
				} else if(light_type == L"point") {
					light->light_type = LightType::POINT;
				} else if(light_type == L"spot") {
					light->light_type = LightType::SPOT;
				}
			}
			if(light_json_object.find(L"intensity") != light_json_object.end() && light_json_object[L"intensity"]->IsArray()) {
				JSONArray color_json_array = light_json_object[L"intensity"]->AsArray();
				if(color_json_array.size() == 3) {
					light->spectrum.r = (float) color_json_array[0]->AsNumber();
					light->spectrum.g = (float) color_json_array[1]->AsNumber();
					light->spectrum.b = (float) color_json_array[2]->AsNumber();
				}
			}
			if(light_json_object.find(L"position") != light_json_object.end() && light_json_object[L"position"]->IsArray()) {
				JSONArray position_json_array = light_json_object[L"position"]->AsArray();
				if(position_json_array.size() == 3) {
					light->position.x = position_json_array[0]->AsNumber();
					light->position.y = position_json_array[1]->AsNumber();
					light->position.z = position_json_array[2]->AsNumber();
				}
			}
			if(light_json_object.find(L"direction") != light_json_object.end() && light_json_object[L"direction"]->IsArray()) {
				JSONArray direction_json_array = light_json_object[L"direction"]->AsArray();
				if(direction_json_array.size() == 3) {
					light->direction.x = direction_json_array[0]->AsNumber();
					light->direction.y = direction_json_array[1]->AsNumber();
					light->direction.z = direction_json_array[2]->AsNumber();
				}
			}
			if(light_json_object.find(L"falloff_deg") != light_json_object.end() && light_json_object[L"falloff_deg"]->IsNumber()) {
				light->falloff_deg = light_json_object[L"falloff_deg"]->AsNumber();
			}
			if(light_json_object.find(L"falloff_exp") != light_json_object.end() && light_json_object[L"falloff_exp"]->IsNumber()) {
				light->falloff_exp = light_json_object[L"falloff_exp"]->AsNumber();
			}
			if(light_json_object.find(L"constant_att") != light_json_object.end() && light_json_object[L"constant_att"]->IsNumber()) {
				light->constant_att = light_json_object[L"constant_att"]->AsNumber();
			}
			if(light_json_object.find(L"linear_att") != light_json_object.end() && light_json_object[L"linear_att"]->IsNumber()) {
				light->linear_att = light_json_object[L"linear_att"]->AsNumber();
			}
			if(light_json_object.find(L"quadratic_att") != light_json_object.end() && light_json_object[L"quadratic_att"]->IsNumber()) {
				light->quadratic_att = light_json_object[L"quadratic_att"]->AsNumber();
			}
			light->up = up;
			light->type = Instance::LIGHT;
			node.instance = light;
			scene->nodes.push_back(node);
		}
    }

    if (root.find(L"meshes") != root.end() && root[L"meshes"]->IsArray()) {
        JSONArray mesh_json_array = root[L"meshes"]->AsArray();
		for(int i = 0; i < mesh_json_array.size(); ++i) {
			if(!mesh_json_array[i]->IsObject()) continue;
			JSONObject mesh_json_object = mesh_json_array[i]->AsObject();
			Node node = Node();
			PolymeshInfo* polymesh = new PolymeshInfo();
            polymesh->position = Vector3D(0,0,0);
            polymesh->rotation = Vector3D(1,1,1);
            polymesh->scale = Vector3D(1,1,1);
            polymesh->is_obj_file = false;
            polymesh->is_mtl_file = false;
            polymesh->is_disney = false;
            polymesh->is_mirror_brdf = false;
            polymesh->phong_spec_exp = 1.f;

			if (mesh_json_object.find(L"use_disney") != mesh_json_object.end() && mesh_json_object[L"use_disney"]->IsString()) {
			    if(L"true" == mesh_json_object[L"use_disney"]->AsString()) {
                    polymesh->is_disney = true;
                }
            }
			if (mesh_json_object.find(L"material_filename") != mesh_json_object.end() && mesh_json_object[L"material_filename"]->IsString()) {
				string material_filename = path + wstring_to_string(mesh_json_object[L"material_filename"]->AsString());
				size_t pos = string::npos;
				size_t pos1 = material_filename.find(".mtl");
				size_t pos2 = material_filename.find(".MTL");
				if(pos1 != string::npos) pos = pos1;
				if(pos2 != string::npos) pos = pos2;
				if(pos != string::npos) {
					pos += 4;
					material_filename = material_filename.substr(0, pos);
					if(material_filename.substr(material_filename.find_last_of(".") + 1) == "mtl"
						|| material_filename.substr(material_filename.find_last_of(".") + 1) == "MTL") {
						ifstream in(material_filename);
						if (!in.is_open()) {
							cerr << "Warning: could not open file " << material_filename << endl;
							return -1;
						}

						if(!parse_mtl(in, *polymesh)) {
							cerr << "Error: bad obj format" << endl;
							in.close();
							return -1;
						}

						polymesh->is_mtl_file = true;
						in.close();
					}
				}
			}

      Vector3D mesh_translate(0,0,0);
      if (mesh_json_object.find(L"translate") != mesh_json_object.end() && mesh_json_object[L"translate"]->IsArray()) {
        JSONArray position_json_array = mesh_json_object[L"translate"]->AsArray();
        if(position_json_array.size() == 3) {
          mesh_translate.x = position_json_array[0]->AsNumber();
          mesh_translate.y = position_json_array[1]->AsNumber();
          mesh_translate.z = position_json_array[2]->AsNumber();
        }
      }

			if (mesh_json_object.find(L"filename") != mesh_json_object.end() && mesh_json_object[L"filename"]->IsString()) {
				string mesh_filename = path + wstring_to_string(mesh_json_object[L"filename"]->AsString());
				size_t pos = string::npos;
				size_t pos1 = mesh_filename.find(".obj");
				size_t pos2 = mesh_filename.find(".OBJ");
				if(pos1 != string::npos) pos = pos1;
				if(pos2 != string::npos) pos = pos2;
				if(pos != string::npos) {
					pos += 4;
					mesh_filename = mesh_filename.substr(0, pos);
					if(mesh_filename.substr(mesh_filename.find_last_of(".") + 1) == "obj"
							|| mesh_filename.substr(mesh_filename.find_last_of(".") + 1) == "OBJ") {
						ifstream in(mesh_filename);
						if (!in.is_open()) {
							cerr << "Warning: could not open file " << mesh_filename << endl;
							return -1;
						}
 
						if(!parse_objmesh(in, *polymesh)) {
							cerr << "Error: bad obj format" << endl;
							in.close();
							return -1;
						}
						in.close();
					}
				}
				pos = string::npos;
				pos1 = mesh_filename.find(".dae");
				pos2 = mesh_filename.find(".DAE");
				if(pos1 != string::npos) pos = pos1;
				if(pos2 != string::npos) pos = pos2;
				if(pos != string::npos) {
					/*
					pos += 4;
					mesh_filename = mesh_filename.substr(0, pos);
					if(mesh_filename.substr(mesh_filename.find_last_of(".") + 1) == "dae"
							|| mesh_filename.substr(mesh_filename.find_last_of(".") + 1) == "DAE") {
						ifstream in(mesh_filename);
						if (!in.is_open()) {
							cerr << "Warning: could not open file " << mesh_filename << endl;
							return -1;
						}
 
						if(!parse_polymesh(in, *polymesh)) {
							cerr << "Error: bad dae format" << endl;
							in.close();
							return -1;
						}
						in.close();
					}
					*/
				}
			}

      if (mesh_json_object.find(L"material") != mesh_json_object.end() && mesh_json_object[L"material"]->IsString()) {
        string material_type = wstring_to_string(mesh_json_object[L"material"]->AsString());
        if (material_type.compare("mirror") == 0) 
          polymesh->is_mirror_brdf = true;
      }

      if (mesh_json_object.find(L"spec_exp") != mesh_json_object.end() && mesh_json_object[L"spec_exp"]->IsNumber()) {
        double spec_exp = mesh_json_object[L"spec_exp"]->AsNumber();
        polymesh->phong_spec_exp = spec_exp;
      }

			if (mesh_json_object.find(L"diffuse_filename") != mesh_json_object.end() && mesh_json_object[L"diffuse_filename"]->IsString()) {
				string diffuse_filename = path + wstring_to_string(mesh_json_object[L"diffuse_filename"]->AsString());
				size_t pos = string::npos;
				size_t pos1 = diffuse_filename.find(".png");
				size_t pos2 = diffuse_filename.find(".PNG");
				if(pos1 != string::npos) pos = pos1;
				if(pos2 != string::npos) pos = pos2;
				if(pos != string::npos) {
					pos += 4;
					polymesh->diffuse_filename = diffuse_filename.substr(0, pos);
 				}
			}
			if (mesh_json_object.find(L"normal_filename") != mesh_json_object.end() && mesh_json_object[L"normal_filename"]->IsString()) {
				string normal_filename = path + wstring_to_string(mesh_json_object[L"normal_filename"]->AsString());
				size_t pos = string::npos;
				size_t pos1 = normal_filename.find(".png");
				size_t pos2 = normal_filename.find(".PNG");
				if(pos1 != string::npos) pos = pos1;
				if(pos2 != string::npos) pos = pos2;
				if(pos != string::npos) {
					pos += 4;
					polymesh->normal_filename = normal_filename.substr(0, pos);
				}
			}
			if (mesh_json_object.find(L"environment_filename") != mesh_json_object.end() && mesh_json_object[L"environment_filename"]->IsString()) {
				string environment_filename = path + wstring_to_string(mesh_json_object[L"environment_filename"]->AsString());
				size_t pos = string::npos;
				size_t pos1 = environment_filename.find(".png");
				size_t pos2 = environment_filename.find(".PNG");
				if(pos1 != string::npos) pos = pos1;
				if(pos2 != string::npos) pos = pos2;
				if(pos != string::npos) {
					pos += 4;
					polymesh->environment_filename = environment_filename.substr(0, pos);
				}
			}
			if (mesh_json_object.find(L"alpha_filename") != mesh_json_object.end() && mesh_json_object[L"alpha_filename"]->IsString()) {
				string alpha_filename = path + wstring_to_string(mesh_json_object[L"alpha_filename"]->AsString());
				size_t pos = string::npos;
				size_t pos1 = alpha_filename.find(".png");
				size_t pos2 = alpha_filename.find(".PNG");
				if(pos1 != string::npos) pos = pos1;
				if(pos2 != string::npos) pos = pos2;
				if(pos != string::npos) {
					pos += 4;
					polymesh->alpha_filename = alpha_filename.substr(0, pos);
				}
			}
			if (mesh_json_object.find(L"stub1_filename") != mesh_json_object.end() && mesh_json_object[L"stub1_filename"]->IsString()) {
				string stub1_filename = path + wstring_to_string(mesh_json_object[L"stub1_filename"]->AsString());
				size_t pos = string::npos;
				size_t pos1 = stub1_filename.find(".png");
				size_t pos2 = stub1_filename.find(".PNG");
				if(pos1 != string::npos) pos = pos1;
				if(pos2 != string::npos) pos = pos2;
				if(pos != string::npos) {
					pos += 4;
					polymesh->stub1_filename = stub1_filename.substr(0, pos);
				}
			}
			if (mesh_json_object.find(L"stub2_filename") != mesh_json_object.end() && mesh_json_object[L"stub2_filename"]->IsString()) {
				string stub2_filename = path + wstring_to_string(mesh_json_object[L"stub2_filename"]->AsString());
				size_t pos = string::npos;
				size_t pos1 = stub2_filename.find(".png");
				size_t pos2 = stub2_filename.find(".PNG");
				if(pos1 != string::npos) pos = pos1;
				if(pos2 != string::npos) pos = pos2;
				if(pos != string::npos) {
					pos += 4;
					polymesh->stub2_filename = stub2_filename.substr(0, pos);
				}
			}
			if (mesh_json_object.find(L"stub3_filename") != mesh_json_object.end() && mesh_json_object[L"stub3_filename"]->IsString()) {
				string stub3_filename = path + wstring_to_string(mesh_json_object[L"stub3_filename"]->AsString());
				size_t pos = string::npos;
				size_t pos1 = stub3_filename.find(".png");
				size_t pos2 = stub3_filename.find(".PNG");
				if(pos1 != string::npos) pos = pos1;
				if(pos2 != string::npos) pos = pos2;
				if(pos != string::npos) {
					pos += 4;
					polymesh->stub3_filename = stub3_filename.substr(0, pos);
				}
			}
			if (mesh_json_object.find(L"vertex_shader_filename") != mesh_json_object.end() && mesh_json_object[L"vertex_shader_filename"]->IsString()) {
				string vert_filename = path + wstring_to_string(mesh_json_object[L"vertex_shader_filename"]->AsString());
				size_t pos = string::npos;
				size_t pos1 = vert_filename.find(".vert");
				size_t pos2 = vert_filename.find(".vert");
				if(pos1 != string::npos) pos = pos1;
				if(pos2 != string::npos) pos = pos2;
				if(pos != string::npos) {
					pos += 5;
					polymesh->vert_filename = vert_filename.substr(0, pos);
				}
			}
			if (mesh_json_object.find(L"fragment_shader_filename") != mesh_json_object.end() && mesh_json_object[L"fragment_shader_filename"]->IsString()) {
				string frag_filename = path + wstring_to_string(mesh_json_object[L"fragment_shader_filename"]->AsString());
				size_t pos = string::npos;
				size_t pos1 = frag_filename.find(".frag");
				size_t pos2 = frag_filename.find(".frag");
				if(pos1 != string::npos) pos = pos1;
				if(pos2 != string::npos) pos = pos2;
				if(pos != string::npos) {
					pos += 5;
					polymesh->frag_filename = frag_filename.substr(0, pos);
				}
			}
			if(mesh_json_object.find(L"position") != mesh_json_object.end() && mesh_json_object[L"position"]->IsArray()) {
				JSONArray position_json_array = mesh_json_object[L"position"]->AsArray();
				if(position_json_array.size() == 3) {
					polymesh->position.x = position_json_array[0]->AsNumber();
					polymesh->position.y = position_json_array[1]->AsNumber();
					polymesh->position.z = position_json_array[2]->AsNumber();
				}
			}
			if(mesh_json_object.find(L"rotation") != mesh_json_object.end() && mesh_json_object[L"rotation"]->IsArray()) {
				JSONArray rotation_json_array = mesh_json_object[L"rotation"]->AsArray();
				if(rotation_json_array.size() == 3) {
					polymesh->rotation.x = rotation_json_array[0]->AsNumber();
					polymesh->rotation.y = rotation_json_array[1]->AsNumber();
					polymesh->rotation.z = rotation_json_array[2]->AsNumber();
				}
			}

      Vector3D mesh_scale(1,1,1);
			if (mesh_json_object.find(L"scale") != mesh_json_object.end() && mesh_json_object[L"scale"]->IsArray()) {
				JSONArray scale_json_array = mesh_json_object[L"scale"]->AsArray();
				if(scale_json_array.size() == 3) {
					mesh_scale.x = polymesh->scale.x = scale_json_array[0]->AsNumber();
					mesh_scale.y = polymesh->scale.y = scale_json_array[1]->AsNumber();
					mesh_scale.z = polymesh->scale.z = scale_json_array[2]->AsNumber();
				}
			}
			if (mesh_json_object.find(L"texcoord_u_scale") != mesh_json_object.end() && mesh_json_object[L"texcoord_u_scale"]->IsNumber()) {
				double scale = mesh_json_object[L"texcoord_u_scale"]->AsNumber();
				for(int i = 0; i < polymesh->texcoords.size(); ++i) {
					polymesh->texcoords[i].x = polymesh->texcoords[i].x * scale;
				}
			}
			if (mesh_json_object.find(L"texcoord_v_wrap") != mesh_json_object.end() && mesh_json_object[L"texcoord_v_wrap"]->IsString()) {
				if(L"true" == mesh_json_object[L"texcoord_v_wrap"]->AsString()) {
					for(int i = 0; i < polymesh->texcoords.size(); ++i) {
						polymesh->texcoords[i].y = 1.0 - polymesh->texcoords[i].y;
					}
				}
			}
			if (mesh_json_object.find(L"texcoord_u_flip") != mesh_json_object.end() && mesh_json_object[L"texcoord_u_flip"]->IsString()) {
				if(L"true" == mesh_json_object[L"texcoord_u_flip"]->AsString()) {
                    for(int i = 0; i < polymesh->texcoords.size(); ++i) {
                        polymesh->texcoords[i].x = 1.0 - polymesh->texcoords[i].x;
                    }
                }
			}
			if (mesh_json_object.find(L"texcoord_v_scale") != mesh_json_object.end() && mesh_json_object[L"texcoord_v_scale"]->IsNumber()) {
				double scale = mesh_json_object[L"texcoord_v_scale"]->AsNumber();
				for(int i = 0; i < polymesh->texcoords.size(); ++i) {
					polymesh->texcoords[i].y = polymesh->texcoords[i].y * scale;
				}
			}
			if (mesh_json_object.find(L"texcoord_v_wrap") != mesh_json_object.end() && mesh_json_object[L"texcoord_v_wrap"]->IsString()) {
				if(L"true" == mesh_json_object[L"texcoord_v_wrap"]->AsString()) {
					for(int i = 0; i < polymesh->texcoords.size(); ++i) {
						polymesh->texcoords[i].y = 1.0 - polymesh->texcoords[i].y;
					}
				}
			}
			if (mesh_json_object.find(L"texcoord_v_flip") != mesh_json_object.end() && mesh_json_object[L"texcoord_v_flip"]->IsString()) {
				if(L"true" == mesh_json_object[L"texcoord_v_flip"]->AsString()) {
                    for(int i = 0; i < polymesh->texcoords.size(); ++i) {
                        polymesh->texcoords[i].y = 1.0 - polymesh->texcoords[i].y;
                    }
                }
            }
			if (mesh_json_object.find(L"parameters") != mesh_json_object.end() && mesh_json_object[L"parameters"]->IsArray()) {
				JSONArray parameters_json_array = mesh_json_object[L"parameters"]->AsArray();
				for(int i = 0; i < parameters_json_array.size(); ++i) {
					if(!parameters_json_array[i]->IsObject()) continue;
					JSONObject parameter_json_object = parameters_json_array[i]->AsObject();
					Node node = Node();
					if (parameter_json_object.find(L"name") != parameter_json_object.end() && parameter_json_object[L"name"]->IsString()) {
						string name = wstring_to_string(parameter_json_object[L"name"]->AsString());
						polymesh->uniform_strings.push_back(name);
					}
					if (parameter_json_object.find(L"value") != parameter_json_object.end() && parameter_json_object[L"value"]->IsNumber()) {
						float value = parameter_json_object[L"value"]->AsNumber();
						polymesh->uniform_values.push_back(value);
					}

				}
			}

			polymesh->type = Instance::POLYMESH;
			node.instance = polymesh;
			//node.transform = Matrix4x4::identity();
      node.transform = Matrix4x4::translation(mesh_translate) * Matrix4x4::scaling(mesh_scale);
			scene->nodes.push_back(node);
		}
    }

      return 0;
  }

  if(filename_test.substr(filename_test.find_last_of(".") + 1) == "obj"
      || filename_test.substr(filename_test.find_last_of(".") + 1) == "OBJ")
  {
      stat("Loading OBJ file...");
      scene = sceneInfo;

      // mesh geometry
      PolymeshInfo* polymesh = new PolymeshInfo();
      if(!parse_objmesh(in, *polymesh))
      {
        cerr << "Error: bad obj format" << endl;
        in.close();
        return -1;
      }
	  in.close();

	  // HardCode begins
	  polymesh->type = Instance::POLYMESH;

	  Node node1 = Node();
	  CameraInfo* camera = new CameraInfo();
	  camera->view_dir = Vector3D(0, 0, -1);
	  camera->up_dir = Vector3D(0,1,0);
	  camera->hFov = 50.0f;
	  camera->vFov = 35.0f;
	  camera->nClip = 0.001f;
	  camera->fClip = 1000.0f;
	  node1.instance = camera;
	  node1.transform = Matrix4x4::identity();

	  scene->nodes.push_back(node1);

	  // TODO: THIS IS SO NEEDED IF WE WANT TO DO PROPER SHADING!!!!
//	  Node node2 = Node();
//	  LightInfo* light = new LightInfo();
//
//	  light->light_type = LightType::POINT;
//			  light->spectrum = Spectrum(1,1,1);
//			  light->position = Vector3D(1, 1, 1);
//
//	  node2.instance = light;
//	  scene->nodes.push_back(node2);

	  Node node3 = Node();
	  node3.instance = polymesh;
	  node3.transform = Matrix4x4::identity();
	  scene->nodes.push_back(node3);
	  // HardCode ends

      return 0;
  }
  in.close();

  XMLDocument doc;
  doc.LoadFile(filename);
  if (doc.Error()) {
    stat("XML error: ");
    doc.PrintError();
    exit(EXIT_FAILURE);
  }

  // Check XML schema
  XMLElement* root = doc.FirstChildElement("COLLADA");
  if (!root) {
    stat("Error: not a COLLADA file!") exit(EXIT_FAILURE);
  } else {
    stat("Loading COLLADA file...");
  }

  // Set output scene pointer
  scene = sceneInfo;

  // Build uri table
  uri_load(root);

  // Load assets - correct up direction
  if (XMLElement* e_asset = get_element(root, "asset")) {
    XMLElement* up_axis = get_element(e_asset, "up_axis");
    if (!up_axis) {
      stat("Error: No up direction defined in COLLADA file");
      exit(EXIT_FAILURE);
    }

    // get up direction and correct non-Y_UP scenes by setting a non-identity
    // global entry transformation, assuming right hand coordinate system for
    // both input and output

    string up_dir = up_axis->GetText();
    transform = Matrix4x4::identity();
    if (up_dir == "X_UP") {
      // swap X-Y and negate Z
      transform(0, 0) = 0;
      transform(0, 1) = 1;
      transform(1, 0) = 1;
      transform(1, 1) = 0;
      transform(2, 2) = -1;

      // local up direction for lights and cameras
      up = Vector3D(1, 0, 0);

    } else if (up_dir == "Z_UP") {
      // swap Z-Y matrix and negate X
      transform(1, 1) = 0;
      transform(1, 2) = 1;
      transform(2, 1) = 1;
      transform(2, 2) = 0;
      transform(0, 0) = -1;

      // local up direction cameras
      up = Vector3D(0, 0, 1);

    } else if (up_dir == "Y_UP") {
      up = Vector3D(0, 1, 0);  // no need to correct Y_UP as its used internally
    } else {
      stat("Error: invalid up direction in COLLADA file");
      exit(EXIT_FAILURE);
    }
  }

  // Load scene -
  // A scene should only have one visual_scene instance, this constraint
  // creates a one-to-one relationship between the document, the top-level
  // scene, and its visual description (COLLADA spec 1.4 page 91)
  if (XMLElement* e_scene = get_element(root, "scene/instance_visual_scene")) {
    stat("Loading scene...");

    // parse all nodes in scene
    XMLElement* e_node = get_element(e_scene, "node");
    while (e_node) {
      parse_node(e_node);
      e_node = e_node->NextSiblingElement("node");
    }

  } else {
    stat("Error: No scene description found in file:" << filename);
    return -1;
  }

  return 0;
}

int ColladaParser::save(const char* filename, const SceneInfo* sceneInfo) {
  // TODO: not yet supported
  return 0;
}

void ColladaParser::parse_node(XMLElement* xml) {
  // create new node
  Node node = Node();

  // name & id
  node.id = xml->Attribute("id");
  node.name = xml->Attribute("name");
  stat(" |- Node: " << node.name << " (id:" << node.id << ")");

  // node transformation -
  // combine in order of declaration if the transformations are given as a
  // transformation list
  XMLElement* e = xml->FirstChildElement();
  while (e) {
    string name = e->Name();

    // transformation - matrix
    if (name == "matrix") {
      string s = e->GetText();
      stringstream ss(s);

      Matrix4x4 mat;
      ss >> mat(0, 0);
      ss >> mat(0, 1);
      ss >> mat(0, 2);
      ss >> mat(0, 3);
      ss >> mat(1, 0);
      ss >> mat(1, 1);
      ss >> mat(1, 2);
      ss >> mat(1, 3);
      ss >> mat(2, 0);
      ss >> mat(2, 1);
      ss >> mat(2, 2);
      ss >> mat(2, 3);
      ss >> mat(3, 0);
      ss >> mat(3, 1);
      ss >> mat(3, 2);
      ss >> mat(3, 3);

      node.transform = mat;
      break;
    }

    // transformation - rotate
    if (name == "rotate") {
      Matrix4x4 m;

      string s = e->GetText();
      stringstream ss(s);

      string sid = e->Attribute("sid");
      switch (sid.back()) {
        case 'X':
          ss >> m(1, 1);
          ss >> m(1, 2);
          ss >> m(2, 1);
          ss >> m(2, 2);
          break;
        case 'Y':
          ss >> m(0, 0);
          ss >> m(2, 0);
          ss >> m(0, 2);
          ss >> m(2, 2);
          break;
        case 'Z':
          ss >> m(0, 0);
          ss >> m(0, 1);
          ss >> m(1, 0);
          ss >> m(1, 1);
          break;
        default:
          break;
      }

      node.transform = m * node.transform;
    }

    // transformation - translate
    if (name == "translate") {
      Matrix4x4 m;

      string s = e->GetText();
      stringstream ss(s);

      ss >> m(0, 3);
      ss >> m(1, 3);
      ss >> m(2, 3);

      node.transform = m * node.transform;
    }

    // transformation - scale
    if (name == "scale") {
      Matrix4x4 m;

      string s = e->GetText();
      stringstream ss(s);

      ss >> m(0, 0);
      ss >> m(1, 1);
      ss >> m(1, 1);

      node.transform = m * node.transform;
    }

    // transformation - skew
    // Note (sky): ignored for now

    // transformation - lookat
    // Note (sky): ignored for now

    e = e->NextSiblingElement();
  }

  // push transformations
  Matrix4x4 transform_save = transform;

  // combine transformations
  node.transform = transform * node.transform;
  transform = node.transform;

  // parse child nodes if node is a joint
  XMLElement* e_child = get_element(xml, "node");
  while (e_child) {
    parse_node(e_child);
    e_child = e_child->NextSiblingElement("node");
  }

  // pop transformations
  transform = transform_save;

  // node instance -
  // non-joint nodes must contain a scene object instance
  XMLElement* e_camera = get_element(xml, "instance_camera");
  XMLElement* e_light = get_element(xml, "instance_light");
  XMLElement* e_geometry = get_element(xml, "instance_geometry");

  if (e_camera) {
    CameraInfo* camera = new CameraInfo();
    parse_camera(e_camera, *camera);
    node.instance = camera;
  } else if (e_light) {
    LightInfo* light = new LightInfo();
    parse_light(e_light, *light);
    node.instance = light;
  } else if (e_geometry) {
    if (get_element(e_geometry, "mesh")) {
      // mesh geometry
      PolymeshInfo* polymesh = new PolymeshInfo();
      parse_polymesh(e_geometry, *polymesh);

      // mesh material
      XMLElement* e_instance_material = get_element(
          xml,
          "instance_geometry/bind_material/technique_common/instance_material");
      if (e_instance_material) {
        if (!e_instance_material->Attribute("target")) {
          stat(
              "Error: no target material in instance: " << e_instance_material);
          exit(EXIT_FAILURE);
        }

        string material_id = e_instance_material->Attribute("target") + 1;
        XMLElement* e_material = uri_find(material_id);
        if (!e_material) {
          stat("Error: invalid target material id : " << material_id);
          exit(EXIT_FAILURE);
        }
      }

      node.instance = polymesh;

    } else if (get_element(e_geometry, "extra")) {
      // sphere geometry
      SphereInfo* sphere = new SphereInfo();
      parse_sphere(e_geometry, *sphere);

      // sphere material
      XMLElement* e_instance_material = get_element(
          xml,
          "instance_geometry/bind_material/technique_common/instance_material");
      if (e_instance_material) {
        if (!e_instance_material->Attribute("target")) {
          stat(
              "Error: no target material in instance: " << e_instance_material);
          exit(EXIT_FAILURE);
        }

        string material_id = e_instance_material->Attribute("target") + 1;
        XMLElement* e_material = uri_find(material_id);
        if (!e_material) {
          stat("Error: invalid target material id : " << material_id);
          exit(EXIT_FAILURE);
        }
      }

      node.instance = sphere;
    }
  }

  // add node to scene
  scene->nodes.push_back(node);
}

void ColladaParser::parse_camera(XMLElement* xml, CameraInfo& camera) {
  // name & id
  camera.id = xml->Attribute("id");
  camera.name = xml->Attribute("name");
  camera.type = Instance::CAMERA;

  // default look direction is down the up axis
  camera.up_dir = up;
  camera.view_dir = Vector3D(0, 0, -1);

  // NOTE (sky): only supports perspective for now
  XMLElement* e_perspective =
      get_element(xml, "optics/technique_common/perspective");
  if (e_perspective) {
    XMLElement* e_xfov = e_perspective->FirstChildElement("xfov");
    XMLElement* e_yfov = e_perspective->FirstChildElement("yfov");
    XMLElement* e_znear = e_perspective->FirstChildElement("znear");
    XMLElement* e_zfar = e_perspective->FirstChildElement("zfar");

    camera.hFov = e_xfov ? atof(e_xfov->GetText()) : 50.0f;
    camera.vFov = e_yfov ? atof(e_yfov->GetText()) : 35.0f;
    camera.nClip = e_znear ? atof(e_znear->GetText()) : 0.001f;
    camera.fClip = e_zfar ? atof(e_zfar->GetText()) : 1000.0f;

    if (!e_yfov) {  // if vfov is not given, compute from aspect ratio
      XMLElement* e_aspect_ratio = get_element(e_perspective, "aspect_ratio");
      if (e_aspect_ratio) {
        float aspect_ratio = atof(e_aspect_ratio->GetText());
        camera.vFov =
            2 * degrees(atan(tan(radians(0.5 * camera.hFov)) / aspect_ratio));
      } else {
        stat("Error: incomplete perspective definition in: " << camera.id);
        exit(EXIT_FAILURE);
      }
    }
  } else {
    stat("Error: no perspective defined in camera: " << camera.id);
    exit(EXIT_FAILURE);
  }

  // print summary
  stat("  |- " << camera);
}

void ColladaParser::parse_light(XMLElement* xml, LightInfo& light) {
  // name & id
  light.id = xml->Attribute("id");
  light.name = xml->Attribute("name");
  light.type = Instance::LIGHT;

  XMLElement* technique = NULL;
  XMLElement* technique_common = get_technique_common(xml);
  XMLElement* technique_cmu462 = get_technique_cmu462(xml);

  technique = technique_cmu462 ? technique_cmu462 : technique_common;
  if (!technique) {
    stat("Error: No supported profile defined in light: " << light.id);
    exit(EXIT_FAILURE);
  }

  // light parameters
  XMLElement* e_light = technique->FirstChildElement();
  if (e_light) {
    // type
    string type = e_light->Name();

    // type-specific parameters
    if (type == "ambient") {
      light.light_type = LightType::AMBIENT;
      XMLElement* e_color = get_element(e_light, "color");
      if (e_color) {
        string color_string = e_color->GetText();
        light.spectrum = spectrum_from_string(color_string);
      } else {
        stat("Error: No color definition in ambient light: " << light.id);
        exit(EXIT_FAILURE);
      }
    } else if (type == "directional") {
      light.light_type = LightType::DIRECTIONAL;
      XMLElement* e_color = get_element(e_light, "color");
      if (e_color) {
        string color_string = e_color->GetText();
        light.spectrum = spectrum_from_string(color_string);
      } else {
        stat("Error: No color definition in directional light: " << light.id);
        exit(EXIT_FAILURE);
      }
    } else if (type == "area") {
      light.light_type = LightType::AREA;
      XMLElement* e_color = get_element(e_light, "color");
      if (e_color) {
        string color_string = e_color->GetText();
        light.spectrum = spectrum_from_string(color_string);
      } else {
        stat("Error: No color definition in area light: " << light.id);
        exit(EXIT_FAILURE);
      }
    } else if (type == "point") {
      light.light_type = LightType::POINT;
      XMLElement* e_color = get_element(e_light, "color");
      XMLElement* e_constant_att = get_element(e_light, "constant_attenuation");
      XMLElement* e_linear_att = get_element(e_light, "linear_attenuation");
      XMLElement* e_quadratic_att =
          get_element(e_light, "quadratic_attenuation");
      if (e_color && e_constant_att && e_linear_att && e_quadratic_att) {
        string color_string = e_color->GetText();
        light.spectrum = spectrum_from_string(color_string);
        light.constant_att = atof(e_constant_att->GetText());
        light.constant_att = atof(e_linear_att->GetText());
        light.constant_att = atof(e_quadratic_att->GetText());
      } else {
        stat("Error: incomplete definition of point light: " << light.id);
        exit(EXIT_FAILURE);
      }
    } else if (type == "spot") {
      light.light_type = LightType::SPOT;
      XMLElement* e_color = get_element(e_light, "color");
      XMLElement* e_falloff_deg = e_light->FirstChildElement("falloff_angle");
      XMLElement* e_falloff_exp =
          e_light->FirstChildElement("falloff_exponent");
      XMLElement* e_constant_att = get_element(e_light, "constant_attenuation");
      XMLElement* e_linear_att = get_element(e_light, "linear_attenuation");
      XMLElement* e_quadratic_att =
          get_element(e_light, "quadratic_attenuation");
      if (e_color && e_falloff_deg && e_falloff_exp && e_constant_att &&
          e_linear_att && e_quadratic_att) {
        string color_string = e_color->GetText();
        light.spectrum = spectrum_from_string(color_string);
        light.constant_att = atof(e_falloff_deg->GetText());
        light.constant_att = atof(e_falloff_exp->GetText());
        light.constant_att = atof(e_constant_att->GetText());
        light.constant_att = atof(e_linear_att->GetText());
        light.constant_att = atof(e_quadratic_att->GetText());
      } else {
        stat("Error: incomplete definition of spot light: " << light.id);
        exit(EXIT_FAILURE);
      }
    } else {
      stat("Error: Light type " << type << " in light: " << light.id
                                << "is not supported");
      exit(EXIT_FAILURE);
    }
  }

  // print summary
  stat("  |- " << light);
}

void ColladaParser::parse_sphere(XMLElement* xml, SphereInfo& sphere) {
  // name & id
  sphere.id = xml->Attribute("id");
  sphere.name = xml->Attribute("name");
  sphere.type = Instance::SPHERE;

  XMLElement* e_technique = get_technique_cmu462(xml);
  if (!e_technique) {
    stat("Error: no 462 profile technique in geometry: " << sphere.id);
    exit(EXIT_FAILURE);
  }

  XMLElement* e_radius = get_element(e_technique, "sphere/radius");
  if (!e_radius) {
    stat("Error: invalid sphere definition in geometry: " << sphere.id);
    exit(EXIT_FAILURE);
  }

  sphere.radius = atof(e_radius->GetText());

  // print summary
  stat("  |- " << sphere);
}

void ColladaParser::parse_polymesh(XMLElement* xml, PolymeshInfo& polymesh) {
  // name & id
  polymesh.id = xml->Attribute("id");
  polymesh.name = xml->Attribute("name");
  polymesh.type = Instance::POLYMESH;


  XMLElement* e_mesh = xml->FirstChildElement("mesh");
  if (!e_mesh) {
    stat("Error: no mesh data defined in geometry: " << polymesh.id);
    exit(EXIT_FAILURE);
  }

  // array sources
  map<string, vector<float> > arr_sources;
  XMLElement* e_source = e_mesh->FirstChildElement("source");
  while (e_source) {
    // source id
    string source_id = e_source->Attribute("id");

    // source float array - other formats not handled
    XMLElement* e_float_array = e_source->FirstChildElement("float_array");
    if (e_float_array) {
      float f;
      vector<float> floats;

      // load float array string
      string s = e_float_array->GetText();
      stringstream ss(s);

      // load float array
      size_t num_floats = e_float_array->IntAttribute("count");
      for (size_t i = 0; i < num_floats; ++i) {
        ss >> f;
        floats.push_back(f);
      }

      // add to array sources
      arr_sources[source_id] = floats;
    }

    // parse next source
    e_source = e_source->NextSiblingElement("source");
  }

  // vertices
  vector<Vector3D> vertices;
  string vertices_id;
  XMLElement* e_vertices = e_mesh->FirstChildElement("vertices");
  if (!e_vertices) {
    stat("Error: no vertices defined in geometry: " << polymesh.id);
    exit(EXIT_FAILURE);
  } else {
    vertices_id = e_vertices->Attribute("id");
  }

  XMLElement* e_input = e_vertices->FirstChildElement("input");
  while (e_input) {
    // input semantic
    string semantic = e_input->Attribute("semantic");

    // semantic - position
    if (semantic == "POSITION") {
      string source = e_input->Attribute("source") + 1;
      if (arr_sources.find(source) != arr_sources.end()) {
        vector<float>& floats = arr_sources[source];
        size_t num_floats = floats.size();
        for (size_t i = 0; i < num_floats; i += 3) {
          Vector3D v = Vector3D(floats[i], floats[i + 1], floats[i + 2]);
          vertices.push_back(v);
        }
      } else {
        stat("Error: undefined input source: " << source);
        exit(EXIT_FAILURE);
      }
    }

    // NOTE (sky) : only positions are handled currently

    e_input = e_input->NextSiblingElement("input");
  }

  // polylist
  XMLElement* e_polylist;
  XMLElement* e_polylistprim = e_mesh->FirstChildElement("polylist");
  XMLElement* e_tri = e_mesh->FirstChildElement("triangles");

  bool is_polylist;

  // Supports both triangles and polylist as primitives
  if (e_polylistprim) {
    e_polylist = e_polylistprim;
    is_polylist = true;
  } else if (e_tri) {
    e_polylist = e_tri;
    is_polylist = false;
  } else {
    stat("Error: Mesh uses neither polylist nor triangles");
    exit(EXIT_FAILURE);
  }
 
  if (e_polylist) {
    // input arrays & array offsets
    bool has_vertex_array = false;
    size_t vertex_offset = 0;
    bool has_normal_array = false;
    size_t normal_offset = 0;
    bool has_texcoord_array = false;
    size_t texcoord_offset = 0;

    // input arr_sources
    XMLElement* e_input = e_polylist->FirstChildElement("input");
    while (e_input) {
      string semantic = e_input->Attribute("semantic");
      string source = e_input->Attribute("source") + 1;
      size_t offset = e_input->IntAttribute("offset");

      // vertex array source
      if (semantic == "VERTEX") {
        has_vertex_array = true;
        vertex_offset = offset;

        if (source == vertices_id) {
          polymesh.vertices.resize(vertices.size());
          copy(vertices.begin(), vertices.end(), polymesh.vertices.begin());
        } else {
          stat("Error: undefined source for VERTEX semantic: " << source);
          exit(EXIT_FAILURE);
        }
      }

      // normal array source
      if (semantic == "NORMAL") {
        has_normal_array = true;
        normal_offset = offset;

        if (arr_sources.find(source) != arr_sources.end()) {
          vector<float>& floats = arr_sources[source];
          size_t num_floats = floats.size();
          for (size_t i = 0; i < num_floats; i += 3) {
            Vector3D n = Vector3D(floats[i], floats[i + 1], floats[i + 2]);
            polymesh.normals.push_back(n);
          }
        } else {
          stat("Error: undefined source for NORMAL semantic: " << source);
          exit(EXIT_FAILURE);
        }
      }

      // texcoord array source
      if (semantic == "TEXCOORD") {
        has_texcoord_array = true;
        texcoord_offset = offset;

        if (arr_sources.find(source) != arr_sources.end()) {
          vector<float>& floats = arr_sources[source];
          size_t num_floats = floats.size();
          for (size_t i = 0; i < num_floats; i += 2) {
            Vector2D n = Vector2D(floats[i], floats[i + 1]);
            polymesh.texcoords.push_back(n);
          }
        } else {
          stat("Error: undefined source for TEXCOORD semantic: " << source);
          exit(EXIT_FAILURE);
        }
      }

      e_input = e_input->NextSiblingElement("input");
    }

    // polygon info
    size_t num_polygons = e_polylist->IntAttribute("count");
    size_t stride = (has_vertex_array ? 1 : 0) + (has_normal_array ? 1 : 0) +
                    (has_texcoord_array ? 1 : 0);

    // create polygon size array and compute size of index array
    vector<size_t> sizes;
    size_t num_indices = 0;

    if (is_polylist) {
      XMLElement* e_vcount = e_polylist->FirstChildElement("vcount");
      if (e_vcount) {
        size_t size;
        string s = e_vcount->GetText();
        stringstream ss(s);
  
        for (size_t i = 0; i < num_polygons; ++i) {
          ss >> size;
          sizes.push_back(size);
          num_indices += size * stride;
        }
  
      } else {
        stat("Error: polygon sizes undefined in geometry: " << polymesh.id);
        exit(EXIT_FAILURE);
      }
    } else {
    // Not polylist, so must be triangles
      size_t size = 3;
      for (size_t i = 0; i < num_polygons; ++i) {
        sizes.push_back(size);
        num_indices += size * stride;
      }
    }
   
    // index array
    vector<size_t> indices;
    XMLElement* e_p = e_polylist->FirstChildElement("p");
    if (e_p) {
      size_t index;
      string s = e_p->GetText();
      stringstream ss(s);

      for (size_t i = 0; i < num_indices; ++i) {
        ss >> index;
        indices.push_back(index);
      }

    } else {
      stat("Error: no index array defined in geometry: " << polymesh.id);
      exit(EXIT_FAILURE);
    }

    // create polygons
    polymesh.polygons.resize(num_polygons);

    // vertex array indices
    if (has_vertex_array) {
      size_t k = 0;
      for (size_t i = 0; i < num_polygons; ++i) {
        for (size_t j = 0; j < sizes[i]; ++j) {
          polymesh.polygons[i].vertex_indices.push_back(
              indices[k * stride + vertex_offset]);
          k++;
        }
      }
    }

    // normal array indices
    if (has_normal_array) {
      size_t k = 0;
      for (size_t i = 0; i < num_polygons; ++i) {
        for (size_t j = 0; j < sizes[i]; ++j) {
          polymesh.polygons[i].normal_indices.push_back(
              indices[k * stride + normal_offset]);
          k++;
        }
      }
    }

    // texcoord array indices
    if (has_normal_array) {
      size_t k = 0;
      for (size_t i = 0; i < num_polygons; ++i) {
        for (size_t j = 0; j < sizes[i]; ++j) {
          polymesh.polygons[i].texcoord_indices.push_back(
              indices[k * stride + texcoord_offset]);
          k++;
        }
      }
    }
  }

  // print summary
  stat("  |- " << polymesh);
}

bool ColladaParser::parse_objmesh(ifstream &in, PolymeshInfo& polymesh) {
  polymesh.is_obj_file = true;
  string line;
  
  while(getline(in, line)) {
    if(line[0] == 'v') {
      if(line[1] == ' ') {
	    const char *cp = &line[2];
        while(*cp == ' ') cp++;
        Vector3D vertex;
        if(sscanf(cp, "%lf %lf %lf", &vertex.x, &vertex.y, &vertex.z) == 3) {
          polymesh.vertices.push_back(vertex);
        }
      } else if(line[1] == 'n') {
	    const char *cp = &line[2];
        while(*cp == ' ') cp++;
        Vector3D normal;
        if(sscanf(cp, "%lf %lf %lf", &normal.x, &normal.y, &normal.z) == 3) {
          polymesh.normals.push_back(normal);
        }
      } else if(line[1] == 't') {
	    const char *cp = &line[2];
        while(*cp == ' ') cp++;
        Vector2D texture_coordinate;
        if(sscanf(cp, "%lf %lf", &texture_coordinate.x, &texture_coordinate.y) == 2) {
          polymesh.texcoords.push_back(texture_coordinate);
        }
      }
    }
  }
  
  in.clear();
  in.seekg(0);
  
  while(getline(in, line)) {
    if(line[0] == 'f' && line[1] == ' ') {
      int vertex_index = 0;
      int normal_index = 0;
      int texture_coordinate_index = 0;
      polymesh.polygons.push_back(Polygon());
      Polygon &poly = polymesh.polygons[polymesh.polygons.size() - 1];
	  const char *cp = &line[2];
      while(*cp == ' ') cp++;
      while(sscanf(cp, "%d//%d", &vertex_index, &normal_index) == 2) {
        poly.vertex_indices.push_back(vertex_index - 1);
		poly.normal_indices.push_back(normal_index - 1);
        while(*cp && *cp != ' ') cp++;
        while(*cp == ' ') cp++;
      }
      while(sscanf(cp, "%d/%d/%d", &vertex_index, &texture_coordinate_index, &normal_index) == 3) {
        poly.vertex_indices.push_back(vertex_index - 1);
        poly.texcoord_indices.push_back(texture_coordinate_index - 1);
        poly.normal_indices.push_back(normal_index - 1);
        while(*cp && *cp != ' ') cp++;
        while(*cp == ' ') cp++;
      }
      while(sscanf(cp, "%d/%d", &vertex_index, &texture_coordinate_index) == 2) {
        poly.vertex_indices.push_back(vertex_index - 1);
        poly.texcoord_indices.push_back(texture_coordinate_index - 1);
        while(*cp && *cp != ' ') cp++;
        while(*cp == ' ') cp++;
      }
      while(sscanf(cp, "%d/", &vertex_index) == 1) {
        poly.vertex_indices.push_back(vertex_index - 1);
        while(*cp && *cp != ' ') cp++;
        while(*cp == ' ') cp++;
      }
	  if(*cp) return false;
      int size = polymesh.polygons[polymesh.polygons.size() - 1].vertex_indices.size();
	  if(size != 3) stat("Non triangle detected");
    }
  }

  in.clear();
  in.seekg(0);

  polymesh.material_diffuse_parameters.resize(polymesh.polygons.size());

  Vector3D diffuse_value = Vector3D();
  int face_id = 0;
  while(getline(in, line)) {
	if(line[0] == 'u' && line[1] == 's' && line[2] == 'e' && line[3] == 'm' && line[4] == 't' && line[5] == 'l' && line[6] == ' ') {
		const char *cp = &line[7];
		while(*cp == ' ') cp++;
		char material_name[255];
		if(sscanf(cp, "%s", material_name) == 1) {
			for(int i = 0; i < polymesh.material_names.size(); ++i) {
				if(polymesh.material_names[i] == string(material_name)) {
					diffuse_value = polymesh.material_diffuse_values[i];
					break;
				}
			}
		}
	} else if(line[0] == 'f' && line[1] == ' ') {
		polymesh.material_diffuse_parameters[face_id++] = diffuse_value;
	}
  }

  return true;
}

bool ColladaParser::parse_mtl(ifstream &in, PolymeshInfo& polymesh) {
	string line;
	while(getline(in, line)) {
		if(line[0] == 'n' && line[1] == 'e' && line[2] == 'w' && line[3] == 'm' && line[4] == 't' && line[5] == 'l' && line[6] == ' ') {
			const char *cp = &line[7];
			while(*cp == ' ') cp++;
			char material_name[255];
			if(sscanf(cp, "%s", material_name) == 1) {
				while(getline(in, line)) {
					if(line[0] == 'K' && line[1] == 'd' && line[2] == ' ') {
						const char *cp = &line[2];
						while(*cp == ' ') cp++;
						vector<double> values;
						double value = 0;
						values.reserve(3);
						while(sscanf(cp, "%lf", &value) == 1) {
							values.push_back(value);
							while(*cp && *cp != ' ') cp++;
							while(*cp == ' ') cp++;
						}
						if(values.size() == 3) {
							Vector3D material_diffuse_value;
							material_diffuse_value.x = values[0];
							material_diffuse_value.y = values[1];
							material_diffuse_value.z = values[2];
							polymesh.material_names.push_back(material_name);
							polymesh.material_diffuse_values.push_back(material_diffuse_value);
						}
						break;
					}
				}
			}
		}
	}

	return true;
}

// ====================================================================
// ============ ColladaWriter =========================================
// ====================================================================


// bool ColladaWriter::writeScene(DynamicScene::Scene& scene,
//                                const char* filename) {
//   ofstream out(filename);
//   if (!out.is_open()) {
//     cerr << "WARNING: Could not open file " << filename
//          << " for COLLADA export!" << endl;
//     return false;
//   }

//   writeHeader(out);
//   writeGeometry(out, scene);
//   // TODO lights, camera, materials
//   writeVisualScenes(out, scene);
//   writeFooter(out);

//   return true;
// }

// void writeCurrentTime(ofstream& out) {
//   auto t = time(nullptr);
//   auto tm = *localtime(&t);

//   out << tm.tm_year + 1900 << "-";
//   if (tm.tm_mon < 10) out << "0";
//   out << tm.tm_mon + 1 << "-";
//   if (tm.tm_mday < 10) out << "0";
//   out << tm.tm_mday << "T";
//   if (tm.tm_hour < 10) out << "0";
//   out << tm.tm_hour << ":";
//   if (tm.tm_min < 10) out << "0";
//   out << tm.tm_min << ":";
//   if (tm.tm_sec < 10) out << "0";
//   out << tm.tm_sec;
// }

// void ColladaWriter::writeHeader(ofstream& out) {
//   out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << endl;
//   out << "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" "
//          "version=\"1.4.1\">"
//       << endl;
//   out << "<asset>" << endl;
//   out << "   <contributor>" << endl;
//   out << "      <author>Cardinal</author>" << endl;
//   out << "      <authoring_tool>Stanford Cardinal3D (version "
//          "CS248)</authoring_tool>"
//       << endl;
//   out << "   </contributor>" << endl;
//   out << "   <created>";
//   writeCurrentTime(out);
//   out << "</created>" << endl;
//   out << "   <modified>";
//   writeCurrentTime(out);
//   out << "</modified>" << endl;
//   out << "   <unit name=\"meter\" meter=\"1\"/>" << endl;
//   out << "   <up_axis>Y_UP</up_axis>" << endl;
//   out << "</asset>" << endl;
// }

// void ColladaWriter::writeFooter(ofstream& out) { out << "</COLLADA>" << endl; }

// void ColladaWriter::writeGeometry(ofstream& out, DynamicScene::Scene& scene) {
//   int nMeshes = 0;

//   std::cout << "writeGeometry(): NYI. Continuing..." << std::endl;

//   out << "   <library_geometries>" << endl;
//   /*
//   for (auto o : scene.objects) {
//     DynamicScene::Mesh* mesh = dynamic_cast<DynamicScene::Mesh*>(o);
//     if (mesh) {
//       nMeshes++;
//       writeMesh(out, mesh, nMeshes);
//     }
//   }
//   */
//   out << "   </library_geometries>" << endl;
// }

// void ColladaWriter::writeMesh(ofstream& out, DynamicScene::Mesh* mesh, int id) {
// 	std::cout << "writeMesh(): NYI. Continuing..." << std::endl;
// }

// void ColladaWriter::writeVisualScenes(ofstream& out,
//                                       DynamicScene::Scene& scene) {
//   out << "   <library_visual_scenes>" << endl;
//   out << "      <visual_scene id=\"CardinalScene\">" << endl;

//   int nMeshes = 0;
//   int nNodes = 0;
//   for (auto o : scene.objects) {
//     DynamicScene::Mesh* mesh = dynamic_cast<DynamicScene::Mesh*>(o);
//     if (mesh) {
//       nMeshes++;
//       nNodes++;
//       out << "         <node id=\"N" << nNodes << "\" name=\"Node" << nNodes
//           << "\">" << endl;
//       out << "            <instance_geometry url=\"#M" << nMeshes << "\">"
//           << endl;
//       out << "            </instance_geometry>" << endl;
//       out << "         </node>" << endl;
//     }
//   }

//   out << "      </visual_scene>" << endl;
//   out << "   </library_visual_scenes>" << endl;

//   out << "   <scene>" << endl;
//   out << "      <instance_visual_scene url=\"#CardinalScene\"/>" << endl;
//   out << "   </scene>" << endl;
// }

}  // namespace Collada
}  // namespace CS248
