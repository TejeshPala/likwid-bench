#include <private/internal-components.h>
static const struct hwloc_component * hwloc_static_components[] = {
  &hwloc_noos_component,
  &hwloc_xml_component,
  &hwloc_synthetic_component,
  &hwloc_xml_nolibxml_component,
  &hwloc_linux_component,
#if defined(__x86_64__) || defined(_M_X64) || defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
  &hwloc_x86_component,
#endif
  NULL
};
