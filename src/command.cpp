#include "header.h"
#include <sstream>

void QeCommand::inputCommand(std::string &command) {
    if (command.empty()) return;

    std::vector<std::string> res;
    std::string result;
    std::stringstream input(command);
    while (input >> result) res.push_back(result);

    if (res[0].compare("scene") == 0) {
        OBJMGR->loadScene(atoi(res[1].c_str()));
    } else if (res[0].compare("resetcamera") == 0) {
        GRAP->getTargetCamera()->reset();
        // if(res.size() >1)	VP->getTargetCamera()->type =
        // QeCameraType(atoi(res[1].c_str()));
    } else if (res[0].compare("showmesh") == 0) {
        VK->bShowMesh = !VK->bShowMesh;
        GRAP->bRecreateRender = true;
    } else if (res[0].compare("shownormal") == 0) {
        VK->bShowNormal = !VK->bShowNormal;
        GRAP->bRecreateRender = true;
    }
}
