#ifndef DATAHANDLER_H
#define DATAHANDLER_H

#include <string>

#include "metadatahandler.h"
#include "vds.h"

namespace vds {

// TODO: Should this be derived from the MetadataHandler? To some extend "Data"
// might be interpreted as more general than MetaData. We could also rename it
// VolumeDataHandler to make the distinction more clear.
class DataHandler {
    private:
    MetadataHandler metadata;

    struct SubVolume {
        struct {
            int lower[OpenVDS::VolumeDataLayout::Dimensionality_Max]{0, 0, 0, 0, 0, 0};
            int upper[OpenVDS::VolumeDataLayout::Dimensionality_Max]{1, 1, 1, 1, 1, 1};
        } bounds;
    };

    public:

    DataHandler(const std::string url, const std::string credentials);

    response getSlice(const ApiAxisName& axisName, const int lineNumber);

    response getFence(
        const CoordinateSystem    coordinateSystem,
        float const *             coordinates,
        const size_t              numberOfPoints,
        const InterpolationMethod interpolationMethod
    );
};

} /* namespace vds */

#endif /* DATAHANDLER_H */
