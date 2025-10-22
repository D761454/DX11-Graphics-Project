#pragma once

#include "GameObject.h"

#include <iostream>
#include <fstream>
//JSON include
#include "JSON\json.hpp"

#include "vector"

//Mask namespace for shorthand
using json = nlohmann::json;

class JSONLoad
{
public:
	void JSONSample(std::string path, std::vector<GameObject>& gameobjects, std::vector<LightTypeInfo>& lights);
};

