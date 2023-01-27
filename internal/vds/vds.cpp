#include "vds.h"

#include <stdio.h>
#include <stdlib.h>

#include <array>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <utility>
#include <cmath>

#include "nlohmann/json.hpp"

#include <OpenVDS/OpenVDS.h>
#include <OpenVDS/KnownMetadata.h>
#include <OpenVDS/IJKCoordinateTransformer.h>

#include "boundingbox.h"
//#include "poststack.h"
#include "axis.h"

using namespace std;

void requestdata_delete(struct requestdata* buf) {
    if (!buf)
        return;

    delete[] buf->data;
    delete[] buf->err;
    *buf = requestdata {};
}

static requestdata wrap_as_requestdata( const nlohmann::json::string_t& dump ) {
    requestdata tmp{ new char[dump.size()], nullptr, dump.size() };
    std::copy(dump.begin(), dump.end(), tmp.data);
    return tmp;
}

nlohmann::json convert_axis_to_json(
    const Axis& axis
) {
    nlohmann::json doc;
    doc = {
        { "annotation", axis.getName()           },
        { "min",        axis.getMin()            },
        { "max",        axis.getMax()            },
        { "samples",    axis.getNumberOfPoints() },
        { "unit",       axis.getUnit()           },
    };
    return doc;
}

struct requestdata handle_error(
    const std::exception& e
) {
    requestdata buf {};
    buf.err = new char[std::strlen(e.what()) + 1];
    std::strcpy(buf.err, e.what());
    return buf;
}

struct requestdata metadata(
    char const * const vds,
    char const * const credentials
) {
    try {
        VDSMetadataHandler vdsMetadata( vds, credentials );

        nlohmann::json meta;
        meta["format"] = vdsMetadata.getFormat();

        meta["crs"] = vdsMetadata.getCRS();

        auto bbox = vdsMetadata.getBoundingBox();
        meta["boundingBox"]["ij"]   = bbox.index();
        meta["boundingBox"]["cdp"]  = bbox.world();
        meta["boundingBox"]["ilxl"] = bbox.annotation();

        //{
        //    auto axis_metadata = poststackdata.get_all_axes_metadata();
        //    std::reverse( axis_metadata.begin(), axis_metadata.end() );
        //    std::for_each( axis_metadata.begin(), axis_metadata.end(),
        //        [&meta](const AxisMetadata& desc) {
        //            meta["axis"].push_back( convert_axis_descriptor_to_json( desc ) );
        //        }
        //    );
        //}
        const Axis inlineAxis = vdsMetadata.getInline();
        meta["axis"].push_back( convert_axis_to_json( inlineAxis ) );
        const Axis crosslineAxis = vdsMetadata.getCrossline();
        meta["axis"].push_back( convert_axis_to_json( crosslineAxis ) );
        const Axis sampleAxis = vdsMetadata.getSample();
        meta["axis"].push_back( convert_axis_to_json( sampleAxis ) );

        return wrap_as_requestdata( meta.dump() );
    } catch (const std::exception& e) {
        return handle_error(e);
    }
}

struct requestdata slice(
    char const * const vds,
    char const * const credentials,
    const int          lineno,
    const ApiAxisName         ax
) {
    try {
        //PostStackHandle poststackdata( vds, credentials );
        //return poststackdata.get_slice(ax, lineno);
        VDSDataHandler vdsData(vds, credentials);
        return vdsData.getSlice( ax, lineno );
    } catch (const std::exception& e) {
        std::cerr << "Fetching error " << e.what() << std::endl;
        return handle_error(e);
    }
}

struct requestdata slice_metadata(
    char const * const vds,
    char const * const credentials,
    const ApiAxisName  ax
) {
    try {
        VDSMetadataHandler vdsMetadata( vds, credentials );

        nlohmann::json meta;
        //meta["format"] = poststackdata.get_format(Channel::Sample);
        meta["format"] = vdsMetadata.getFormat();

        //auto axis_metadata = poststackdata.get_all_axes_metadata();
        //{
        //    const std::string axis_name_to_delete = poststackdata.get_axis(ax).name();
        //    auto axis_to_delete = std::find_if(
        //                            axis_metadata.begin(),
        //                            axis_metadata.end(),
        //                            [&axis_name_to_delete](const AxisMetadata& am) {
        //                                return am.name() == axis_name_to_delete;
        //                            }
        //                        );
        //    axis_metadata.erase(axis_to_delete);
        //}

        if (ax == ApiAxisName::I or ax == ApiAxisName::INLINE) {
            meta["x"] = convert_axis_to_json(vdsMetadata.getCrossline());
            meta["y"] = convert_axis_to_json(vdsMetadata.getSample());
        }
        else {
            if (ax == ApiAxisName::J or ax == ApiAxisName::CROSSLINE) {
                meta["x"] = convert_axis_to_json(vdsMetadata.getInline());
                meta["y"] = convert_axis_to_json(vdsMetadata.getSample());
            }
            else {
                meta["x"] = convert_axis_to_json(vdsMetadata.getInline());
                meta["y"] = convert_axis_to_json(vdsMetadata.getCrossline());
            }
        }

        //if (ax == ApiAxisName::I or ax == ApiAxisName::INLINE) {
        //    meta["x"] = convert_axis_descriptor_to_json( vdsMetadata.getCrossline() );
        //    meta["y"] = convert_axis_descriptor_to_json( vdsMetadata.getSample() );
        //}

        auto bbox = vdsMetadata.getBoundingBox();
        meta["boundingBox"]["ij"]   = bbox.index();
        meta["boundingBox"]["cdp"]  = bbox.world();
        meta["boundingBox"]["ilxl"] = bbox.annotation();

        return wrap_as_requestdata( meta.dump() );
    } catch (const std::exception& e) {
        return handle_error(e);
    }
}

struct requestdata fence(
    char const * const             vds,
    char const * const             credentials,
    const enum CoordinateSystem    coordinate_system,
    float const * const            coordinates,
    const size_t                   npoints,
    const enum InterpolationMethod interpolation_method
) {
    try {
        //PostStackHandle poststackdata( vds, credentials );
        //return poststackdata.get_fence(
        //    coordinate_system,
        //    coordinates,
        //    npoints,
        //    interpolation_method );
        VDSDataHandler vdsData( vds, credentials );
        return vdsData.getFence(
            coordinate_system,
            coordinates,
            npoints,
            interpolation_method
        );
    } catch (const std::exception& e) {
        return handle_error(e);
    }
}

struct requestdata fence_metadata(
    char const * const vds,
    char const * const credentials,
    const size_t       npoints
) {
    try {
        //PostStackHandle poststackdata( vds, credentials );
        VDSMetadataHandler vdsMetadata( vds, credentials );

        nlohmann::json meta;
        //{
        //    const auto axis_metadata = poststackdata.get_all_axes_metadata();
        //    meta["shape"] = nlohmann::json::array(
        //                        {npoints, axis_metadata.front().number_of_samples() }
        //                    );
        //}
        const Axis sampleAxis = vdsMetadata.getSample();
        meta["shape"] = nlohmann::json::array(
                            {npoints, sampleAxis.getNumberOfPoints() }
                        );
        meta["format"] = vdsMetadata.getFormat();

        return wrap_as_requestdata( meta.dump() );
    } catch (const std::exception& e) {
        return handle_error(e);
    }
}
