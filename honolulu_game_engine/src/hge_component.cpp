#include <hge_component.hpp>

namespace hnll {

HgeComponent::HgeComponent()
{
  static compId id = 0;
  id_m = id++;
}
} // namespace hnll