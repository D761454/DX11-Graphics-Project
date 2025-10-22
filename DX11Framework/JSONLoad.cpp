#include "JSONLoad.h"

void JSONLoad::JSONSample(std::string path, std::vector<GameObject>& gameobjects, std::vector<LightTypeInfo>& lights)
{
	std::cout << "\nJson Example\n" << std::endl;

	//Json Parser
	json jFile;

	std::ifstream fileOpen("JSON/" + path);

	jFile = json::parse(fileOpen);

	std::string v = jFile["version"].get<std::string>();

	json& objects = jFile["objects"];
	int size = objects.size();
	for (unsigned int i = 0; i < size; i++)
	{
		GameObject g;
		json& objectDesc = objects.at(i);
		g.m_objFile = objectDesc["File"]; // ← gets a string

		g.m_blender = objectDesc["Blender"];

		// used for vs and ps
		g.m_hasTex = objectDesc["HasTex"];
		g.m_hasSpec = objectDesc["HasSpec"];
		g.m_hasNorm = objectDesc["HasNorm"];

		if (g.m_hasTex == 1) {
			std::string in = objectDesc["TextureC"];
			std::wstring temp(in.begin(), in.end());
			g.m_textureColor = temp;
		}
		if (g.m_hasSpec == 1) {
			std::string in = objectDesc["TextureS"];
			std::wstring temp(in.begin(), in.end());
			g.m_textureSpecular = temp;
		}
		if (g.m_hasNorm == 1) {
			std::string in = objectDesc["TextureN"];
			std::wstring temp(in.begin(), in.end());
			g.m_textureNormal = temp;
		}

		gameobjects.push_back(g);
	}

	objects = jFile["lights"];
	size = objects.size();
	for (unsigned int i = 0; i < size; i++)
	{
		LightTypeInfo g;
		json& objectDesc = objects.at(i);
		g.m_type = objectDesc["Type"];
		g.m_color.x = objectDesc["R"];
		g.m_color.y = objectDesc["G"];
		g.m_color.z = objectDesc["B"];
		g.m_color.w = objectDesc["A"];

		lights.push_back(g);
	}

	fileOpen.close();
}