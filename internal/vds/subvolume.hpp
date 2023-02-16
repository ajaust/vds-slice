#ifndef VDS_SLICE_SUBVOLUME_HPP
#define VDS_SLICE_SUBVOLUME_HPP

#include <OpenVDS/OpenVDS.h>

#include "axis.hpp"
#include "metadatahandle.hpp"
#include "vds.h"

struct SubVolume {
    struct {
        int lower[OpenVDS::VolumeDataLayout::Dimensionality_Max]{0, 0, 0, 0, 0, 0};
        int upper[OpenVDS::VolumeDataLayout::Dimensionality_Max]{1, 1, 1, 1, 1, 1};
    } bounds;

    SubVolume(MetadataHandle const& metadata);

    void set_slice(
        Axis const&                  axis,
        int const                    lineno,
        enum coordinate_system const coordinate_system
    );
};

#endif /* VDS_SLICE_SUBVOLUME_HPP */
