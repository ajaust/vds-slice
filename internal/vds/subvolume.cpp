#include "subvolume.hpp"

#include "axis.hpp"
#include "metadatahandle.hpp"
#include "vds.h"

namespace {

int lineno_annotation_to_voxel(
    int lineno,
    Axis const& axis
) {
    /* Assume that annotation coordinates are integers */
    int min      = axis.min();
    int max      = axis.max();
    int nsamples = axis.nsamples();

    auto stride = (max - min) / (nsamples - 1);

    if (lineno < min || lineno > max || (lineno - min) % stride) {
        throw std::runtime_error(
            "Invalid lineno: " + std::to_string(lineno) +
            ", valid range: [" + std::to_string(min) +
            ":" + std::to_string(max) +
            ":" + std::to_string(stride) + "]"
        );
    }

    int voxelline = (lineno - min) / stride;
    return voxelline;
}

int lineno_index_to_voxel(
    int lineno,
    Axis const& axis
) {
    /* Line-numbers in IJK match Voxel - do bound checking and return*/
    int min = 0;
    int max = axis.nsamples() - 1;

    if (lineno < min || lineno > max) {
        throw std::runtime_error(
            "Invalid lineno: " + std::to_string(lineno) +
            ", valid range: [" + std::to_string(min) +
            ":" + std::to_string(max) +
            ":1]"
        );
    }

    return lineno;
}

} /* namespace */

SubVolume::SubVolume(MetadataHandle const& metadata) {
    auto const& iline  = metadata.iline();
    auto const& xline  = metadata.xline();
    auto const& sample = metadata.sample();

    this->bounds.upper[iline.dimension() ] = iline.nsamples();
    this->bounds.upper[xline.dimension() ] = xline.nsamples();
    this->bounds.upper[sample.dimension()] = sample.nsamples();
}

void SubVolume::set_slice(
    Axis const&                  axis,
    int const                    lineno,
    enum coordinate_system const coordinate_system
) {
    int voxelline;
    switch (coordinate_system) {
        case ANNOTATION: {
            voxelline = ::lineno_annotation_to_voxel(lineno, axis);
            break;
        }
        case INDEX: {
            voxelline = ::lineno_index_to_voxel(lineno, axis);
            break;
        }
        default: {
            throw std::runtime_error("Unhandled coordinate system");
        }
    }

    this->bounds.lower[axis.dimension()] = voxelline;
    this->bounds.upper[axis.dimension()] = voxelline + 1;
}
