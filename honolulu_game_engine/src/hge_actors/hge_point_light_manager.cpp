#include <hge_actors/hge_point_light_manager.hpp>

namespace hnll {

HgePointLightManager::HgePointLightManager(GlobalUbo& ubo) : ubo_(ubo)
{

}

void HgePointLightManager::updateActor(float dt)
{
  int lightIndex = 0;
  auto lightRotation = glm::rotate(glm::mat4(1), dt, {0.f, -1.0f, 0.f});
  for (auto& kv : lightCompMap_) {
    auto lightComp = kv.second;
    // update light position
    // lightComp->getTransform().translation_m = glm::vec3(lightRotation * glm::vec4(lightComp->getTransform().translation_m, 1.f));
    // copy light data to ubo 
    ubo_.pointLights[lightIndex].position = glm::vec4(lightComp->getTransform().translation_m, 1.f);
    ubo_.pointLights[lightIndex].color = glm::vec4(lightComp->getColor(), lightComp->getLightInfo().lightIntensity_m);
    lightIndex++;
  }
  ubo_.numLights = lightIndex;
}

} // namespace hnll