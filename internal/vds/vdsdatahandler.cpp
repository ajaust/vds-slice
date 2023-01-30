#include "vdsdatahandler.h"

#include <list>
#include <unordered_map>

#include <OpenVDS/KnownMetadata.h>

VDSDataHandler::VDSDataHandler(
    const std::string url,
    const std::string credentials
) : metadata(url, credentials) {
}

requestdata VDSDataHandler::getSlice(
    const ApiAxisName& axisName,
    const int          lineNumber
) {
    const Axis axis = this->metadata.getAxis(axisName);
    // Validate request axis name and unit name
    if (   axisName == ApiAxisName::DEPTH
        or axisName == ApiAxisName::TIME
        or axisName == ApiAxisName::SAMPLE)
    {
        const std::string axis_unit = axis.getUnit();
        const std::string axis_name = axis.getApiName();

        static const std::unordered_map<std::string, std::list<std::string>>
            allowed_sample_axis_combinations
            = {
                {
                    std::string( OpenVDS::KnownAxisNames::Depth() ),
                    {
                        std::string(OpenVDS::KnownUnitNames::Meter()),
                        std::string(OpenVDS::KnownUnitNames::Foot()),
                        std::string(OpenVDS::KnownUnitNames::USSurveyFoot())
                    }
                },
                {
                    std::string( OpenVDS::KnownAxisNames::Time() ),
                    {
                        std::string(OpenVDS::KnownUnitNames::Millisecond()),
                        std::string(OpenVDS::KnownUnitNames::Second())
                    }
                },
                {
                    std::string( OpenVDS::KnownAxisNames::Sample() ),
                    {
                        std::string(OpenVDS::KnownUnitNames::Unitless())
                    }
                }
            };

        const auto axis_it = allowed_sample_axis_combinations.find(axis_name);

        // This checks for legal name
        if (axis_it == allowed_sample_axis_combinations.end()) {
            const std::string msg = "Unable to use " + axis_name +
                    " on cube with depth units: " + axis_unit;
            throw std::runtime_error(msg);
        }

        const std::list<std::string>& units = axis_it->second;
        auto legal_units_contain = [&units] (const std::string name) {
            return std::find(units.begin(), units.end(), name) != units.end();
        };
        // This checks for legal unit
        if (not legal_units_contain(axis_unit)) {
            const std::string msg = "Unable to use " + axis_name +
                    " on cube with depth units: " + axis_unit;
            throw std::runtime_error(msg);
        }
    }


    //const Axis axis = this->metadata.getAxis(axisName);
    // Set up subvolume
    SubVolume subvolume;
    auto vdsAccessManager = OpenVDS::GetAccessManager(*metadata.getVDSHandle());
    //auto const * vdsLayout = accessManager.GetVolumeDataLayout();
//
    //for (int i = 0; i < 3; ++i) {
    //    subvolume.bounds.upper[i] = vdsLayout->GetDimensionNumSamples(i);
    //}
//
    //for (int i = 0; i < 3; ++i) {
    //    subvolume.bounds.upper[i] = vdsLayout->GetDimensionNumSamples(i);
    //}
    //TODO: Do we prefer this more explicit way of setting everything up?
    //      This should also work in case we do not have the standard VDS layout
    //      created by SEGYImport which is Sample, Crossline, Inline.

    const auto axisBoundsToInitialize = {
                                            ApiAxisName::INLINE,
                                            ApiAxisName::CROSSLINE,
                                            ApiAxisName::SAMPLE
                                        };
    for (const auto axisName: axisBoundsToInitialize) {
        const Axis tmpAxis = metadata.getAxis(axisName);
        subvolume.bounds.upper[tmpAxis.getVdsIndex()] = tmpAxis.getNumberOfPoints();
    }

    //const Axis inlineAxis = metadata.getInline();
    //subvolume.bounds.upper[inlineAxis.getVdsIndex()] = inlineAxis.getNumberOfPoints();
//
    //const Axis crosslineAxis = metadata.getCrossline();
    //subvolume.bounds.upper[crosslineAxis.getVdsIndex()] = crosslineAxis.getNumberOfPoints();
//
    //const Axis sampleAxis = metadata.getSample();
    //subvolume.bounds.upper[sampleAxis.getVdsIndex()] = sampleAxis.getNumberOfPoints();

    // TODO: Check coordinate system? We might be able to skip it?!

    // What line of the  voxel to take?!
    //const int voxelline = to_voxel( axis_desc, lineno );

    const int numberOfPoints = axis.getNumberOfPoints();
    int min    = axis.getMin();
    int max    = axis.getMax();
    int stride = (max - min) / (numberOfPoints - 1);

    // TODO: Should we rather check whether the coordinate system IS Index?!
    if (axis.getCoordinateSystem() != CoordinateSystem::ANNOTATION) {
        min    = 0;
        max    = numberOfPoints - 1;
        stride = 1;
    }

    if (lineNumber < min || lineNumber > max || (lineNumber - min) % stride ) {
        std::string msg = "Invalid lineno: " + std::to_string(lineNumber) +
                          ", valid range: [" + std::to_string(min) +
                          ":" + std::to_string(max);
        if  (axis.getCoordinateSystem() == CoordinateSystem::ANNOTATION) {
            msg += ":" + std::to_string(stride) + "]";
        }
        else {
            msg += ":1]";
        }
        throw std::runtime_error(msg);
    }
    const int voxelline = (lineNumber - min) / stride;

    const int vdsAxisIndex = axis.getVdsIndex();
    subvolume.bounds.lower[vdsAxisIndex] = voxelline;
    subvolume.bounds.upper[vdsAxisIndex] = voxelline + 1;

    // Request data
    //TODO: Maybe obtain this via a call to the metadata object?!
    //      In that case it would also be enough if the VDSMetaDataHandler
    //      would only return a VDSAccessManager.
    const int channelIndex = 0;
    const int levelOfDetailLevel = 0;
    const auto format = metadata.getChannelFormat(channelIndex);
    const auto size = vdsAccessManager.GetVolumeSubsetBufferSize(
                          subvolume.bounds.lower,
                          subvolume.bounds.upper,
                          format,
                          levelOfDetailLevel,
                          channelIndex
                      );

    std::unique_ptr<char[]> data(new char[size]());
    auto request = vdsAccessManager.RequestVolumeSubset(
                       data.get(),
                       size,
                       OpenVDS::Dimensions_012,
                       levelOfDetailLevel,
                       channelIndex,
                       subvolume.bounds.lower,
                       subvolume.bounds.upper,
                       format
                   );

    const bool success = request.get()->WaitForCompletion();
    if (not success) {
        throw std::runtime_error("Failed to fetch slice from VDS");
    }

    requestdata tmp{data.get(), nullptr, static_cast<unsigned long>(size)};
    data.release();

    return tmp;
}


requestdata VDSDataHandler::getFence(
    const CoordinateSystem    coordinateSystem,
    float const *             coordinates,
    const size_t              numberOfPoints,
    const InterpolationMethod interpolationMethod
) {
    auto vdsHandle = metadata.getVDSHandle();
    auto vdsAccessManager = OpenVDS::GetAccessManager(*vdsHandle);
    auto const * vdsLayout = vdsAccessManager.GetVolumeDataLayout();

    //Get point list
    std::unique_ptr< float[][OpenVDS::Dimensionality_Max] > coords(
        new float[numberOfPoints][OpenVDS::Dimensionality_Max]{{0}}
    );

    auto coordinate_transformer = OpenVDS::IJKCoordinateTransformer(vdsLayout);
    auto transform_coordinate = [&] (const float x, const float y) {
        switch (coordinateSystem) {
            case INDEX:
                return OpenVDS::Vector<double, 3> {x, y, 0};
            case ANNOTATION:
                return coordinate_transformer.AnnotationToIJKPosition({x, y, 0});
            case CDP:
                return coordinate_transformer.WorldToIJKPosition({x, y, 0});
            default: {
                throw std::runtime_error("Unhandled coordinate system");
            }
        }
    };

    for (size_t i = 0; i < numberOfPoints; i++) {
        const float x = *(coordinates++);
        const float y = *(coordinates++);

        auto coordinate = transform_coordinate(x, y);

        auto validate_boundary = [&] (const Axis& axis) {
            const auto min = -0.5;
            //const int voxel_dim = this->axis_map_->dimension_from(voxel);
            //const AxisMetadata axis_meta( this->layout_, voxel_dim );
            //const auto max = axis_meta.number_of_samples() - 0.5;
            const auto max = axis.getNumberOfPoints() - 0.5;
            const int apiIndex = axis.getApiIndex();
            if(coordinate[apiIndex] < min || coordinate[apiIndex] >= max) {
                const std::string coordinate_str =
                    "(" +std::to_string(x) + "," + std::to_string(y) + ")";
                throw std::runtime_error(
                    "Coordinate " + coordinate_str + " is out of boundaries "+
                    "in dimension "+ std::to_string(apiIndex)+ "."
                );
            }
        };


        //validate_boundary(axis_dir);
        //Axis inlineAxis = metadata.getInline();
        //Axis crosslineAxis = metadata.getCrossline();

        //for (const Axis& axis: {metadata.getInline(), metadata.getCrossline()} ) {
        Axis inlineAxis = metadata.getInline();
        Axis crosslineAxis = metadata.getCrossline();

        validate_boundary(inlineAxis);
        validate_boundary(crosslineAxis);
        /* openvds uses rounding down for Nearest interpolation.
            * As it is counterintuitive, we fix it by snapping to nearest index
            * and rounding half-up.
            */
        if (interpolationMethod == NEAREST) {
            coordinate[0] = std::round(coordinate[0] + 1) - 1;
            coordinate[1] = std::round(coordinate[1] + 1) - 1;
        }
        coords[i][inlineAxis.getVdsIndex()]    = coordinate[0];
        coords[i][crosslineAxis.getVdsIndex()] = coordinate[1];
        //}
    }

    //Get volume trace
    // TODO: Verify that trace dimension is always 0
    const int channelIndex = 0;
    const int levelOfDetailLevel = 0;
    const int traceDimension = 0;
    const auto size = vdsAccessManager.GetVolumeTracesBufferSize(
        numberOfPoints,
        traceDimension,
        levelOfDetailLevel,
        channelIndex
    );

    std::unique_ptr< char[] > data(new char[size]());

    auto request = vdsAccessManager.RequestVolumeTraces(
                        (float*)data.get(),
                        size,
                        OpenVDS::Dimensions_012,
                        levelOfDetailLevel,
                        channelIndex,
                        coords.get(),
                        numberOfPoints,
                        VDSMetadataHandler::getInterpolation(interpolationMethod),
                        0 // Replacement value
                    );

    const bool success = request.get()->WaitForCompletion();
    if (not success) {
        throw std::runtime_error("Failed to fetch fence from VDS");
    }

    requestdata tmp{data.get(), nullptr, static_cast<unsigned long>(size)};
    data.release();

    return tmp;
}
