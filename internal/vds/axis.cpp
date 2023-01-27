#include "axis.h"

#include <OpenVDS/KnownMetadata.h>

Axis::Axis(
        const ApiAxisName apiAxisName,
        const OpenVDS::VolumeDataLayout* layout
    ) {

    char const * axisNameToFind;
    switch (apiAxisName) {
        case I:
        case INLINE:
            axisNameToFind = OpenVDS::KnownAxisNames::Inline();
        case J:
        case CROSSLINE:
            axisNameToFind = OpenVDS::KnownAxisNames::Crossline();
        case K:
        case DEPTH:
        case TIME:
        case SAMPLE:
            axisNameToFind = OpenVDS::KnownAxisNames::Sample();
        default: {
            throw std::runtime_error("Unhandled axis");
        }
    }

    switch (apiAxisName) {
        case I:
        case J:
        case K:
            this->coordinateSystem = CoordinateSystem::INDEX;
            break;
        case INLINE:
        case CROSSLINE:
        case DEPTH:
        case TIME:
        case SAMPLE:
            this->coordinateSystem = CoordinateSystem::ANNOTATION;
            break;
        default: {
            throw std::runtime_error("Unhandled axis");
        }
    }

    for (int vdsAxisIdx = 0; vdsAxisIdx < 3; ++vdsAxisIdx) {
        const auto& vdsAxisDescriptor = layout->GetAxisDescriptor(vdsAxisIdx);

        const bool names_are_equal = strcmp(
                                        axisNameToFind,
                                        vdsAxisDescriptor.GetName()
                                      ) == 0;

        if ( names_are_equal ) {
            vdsIndex = vdsAxisIdx;
            break;
        }
    }
    vdsAxisDescriptor = layout->GetAxisDescriptor(vdsIndex);

    //Validate expections here now
    if (this->coordinateSystem == CoordinateSystem::ANNOTATION) {
        auto transformer = OpenVDS::IJKCoordinateTransformer(layout);
        if (not transformer.AnnotationsDefined()) {
            throw std::runtime_error("VDS doesn't define annotations");
        }
    }

    if (this->coordinateSystem != CoordinateSystem::ANNOTATION and
        this->coordinateSystem != CoordinateSystem::INDEX ) {
        throw std::runtime_error("Unhandled coordinate system");
    }

}

int Axis::getMin() const {
    return this->vdsAxisDescriptor.GetCoordinateMin();
}

int Axis::getMax() const {
    return this->vdsAxisDescriptor.GetCoordinateMax();
}

int Axis::getNumberOfPoints() const {
    return this->vdsAxisDescriptor.GetNumSamples();
}

std::string Axis::getName() const {
    std::string name = this->vdsAxisDescriptor.GetName();
    return name;
}

std::string Axis::getUnit() const {
    std::string unit = this->vdsAxisDescriptor.GetUnit();
    return unit;
}

int Axis::getVDSIndex() const {
    return this->vdsIndex;
}

CoordinateSystem Axis::getCoordinateSystem() const {
    return this->coordinateSystem;
}

/******************************************************************************/
/* VDSMetadataHandler                                                         */
/******************************************************************************/


VDSMetadataHandler::VDSMetadataHandler(
    const std::string url,
    const std::string credentials
)  {
    OpenVDS::Error error;
    this->vdsHandle = OpenVDS::Open( url, credentials, error );
    if(error.code != 0) {
        throw std::runtime_error("Could not open VDS: " + error.string);
    }

    auto accessManager = OpenVDS::GetAccessManager(this->vdsHandle);
    this->vdsLayout = accessManager.GetVolumeDataLayout();

    //Verify assumptions
    const int expectedDimensionality = 3;
    if (vdsLayout->GetDimensionality() != expectedDimensionality) {
        throw std::runtime_error(
            "Unsupported VDS, expected 3 dimensions, got " +
            std::to_string(this->vdsLayout->GetDimensionality())
        );
    }
}

//VDSMetadataHandler::VDSMetadataHandler( OpenVDS::ScopedVDSHandle vdsHandle ) //: this->vdsHandle(vdsHandle)
//{
//    this->vdsHandle = vdsHandle;
//    auto accessManager = OpenVDS::GetAccessManager(this->vdsHandle);
//    this->vdsLayout = accessManager.GetVolumeDataLayout();
//
//    //Verify assumptions
//    const int expectedDimensionality = 3;
//    if (vdsLayout->GetDimensionality() != expectedDimensionality) {
//        throw std::runtime_error(
//            "Unsupported VDS, expected 3 dimensions, got " +
//            std::to_string(this->vdsLayout->GetDimensionality())
//        );
//    }
//}

// Could also have a member of Axis
Axis VDSMetadataHandler::getInline() const {
    return Axis(ApiAxisName::INLINE, this->vdsLayout);
}
Axis VDSMetadataHandler::getCrossline() const {
    return Axis(ApiAxisName::CROSSLINE, this->vdsLayout);
}
Axis VDSMetadataHandler::getSample() const {
    return Axis(ApiAxisName::SAMPLE, this->vdsLayout);
}

BoundingBox VDSMetadataHandler::getBoundingBox() const {
    return BoundingBox(this->vdsLayout);
}

std::string VDSMetadataHandler::getFormat() const {
    return "<f4";
}

std::string VDSMetadataHandler::getCRS() const {
    const auto crs = OpenVDS::KnownMetadata::SurveyCoordinateSystemCRSWkt();
    return this->vdsLayout->GetMetadataString(crs.GetCategory(), crs.GetName());
}

Axis VDSMetadataHandler::getAxis(const ApiAxisName an) {
    // Do we need to check this here AND when creating the axis?
    //switch(an) {
    //    case ApiAxisName::I:
    //    case ApiAxisName::INLINE:
    //        return this->getInline();
    //    case ApiAxisName::J:
    //    case ApiAxisName::CROSSLINE:
    //        return this->getCrossline();
    //    case ApiAxisName::DEPTH:
    //    case ApiAxisName::SAMPLE:
    //    case ApiAxisName::TIME:
    //        return this->getSample();
    //}
    return Axis( an, this->vdsLayout );
}


OpenVDS::ScopedVDSHandle& VDSMetadataHandler::getVDSHandle() {
    return this->vdsHandle;
}

OpenVDS::InterpolationMethod VDSMetadataHandler::getInterpolation(
    InterpolationMethod interpolation
) {
    switch (interpolation)
    {
        case NEAREST: return OpenVDS::InterpolationMethod::Nearest;
        case LINEAR: return OpenVDS::InterpolationMethod::Linear;
        case CUBIC: return OpenVDS::InterpolationMethod::Cubic;
        case ANGULAR: return OpenVDS::InterpolationMethod::Angular;
        case TRIANGULAR: return OpenVDS::InterpolationMethod::Triangular;
        default: {
            throw std::runtime_error("Unhandled interpolation method");
        }
    }
}


/******************************************************************************/
/* SubVolume                                                                   */
/******************************************************************************/
struct SubVolume {
    struct {
        int lower[OpenVDS::VolumeDataLayout::Dimensionality_Max]{0, 0, 0, 0, 0, 0};
        int upper[OpenVDS::VolumeDataLayout::Dimensionality_Max]{1, 1, 1, 1, 1, 1};
    } bounds;
};

/******************************************************************************/
/* VDSDataHandler                                                             */
/******************************************************************************/

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

    // Set up subvolume
    SubVolume subvolume;
    auto accessManager = OpenVDS::GetAccessManager(metadata.getVDSHandle());
    auto const * vdsLayout = accessManager.GetVolumeDataLayout();

    for (int i = 0; i < 3; ++i) {
        subvolume.bounds.upper[i] = vdsLayout->GetDimensionNumSamples(i);
    }

    for (int i = 0; i < 3; ++i) {
        subvolume.bounds.upper[i] = vdsLayout->GetDimensionNumSamples(i);
    }

    // TODO: Check coordinate system? We might be able to skip it?!

    // What line of the  voxel to take?!
    //const int voxelline = to_voxel( axis_desc, lineno );

    const int numberOfPoints = axis.getNumberOfPoints();
    int min    = axis.getMin();
    int max    = axis.getMax();
    int stride = (max - min) / (numberOfPoints - 1);

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

    const int vdsAxisIndex = axis.getVDSIndex();
    subvolume.bounds.lower[vdsAxisIndex] = voxelline;
    subvolume.bounds.upper[vdsAxisIndex] = voxelline + 1;

    // Request data
    //const auto format = this->metadata.getFormat();
    const auto format = vdsLayout->GetChannelFormat(0);
    const auto size = accessManager.GetVolumeSubsetBufferSize(
                        subvolume.bounds.lower,
                        subvolume.bounds.upper,
                        format,
                        0, //Level of detail level
                        0  // Channel
                    );

    std::unique_ptr<char[]> data(new char[size]());
    auto request = accessManager.RequestVolumeSubset(
                        data.get(),
                        size,
                        OpenVDS::Dimensions_012,
                        0, //Level of detail level
                        0,  // Channel
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
    OpenVDS::ScopedVDSHandle& vdsHandle = metadata.getVDSHandle();
    auto vdsAccessManager = OpenVDS::GetAccessManager(vdsHandle);
    auto vdsLayout = vdsAccessManager.GetVolumeDataLayout();

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
            const int vdsIndex = axis.getVDSIndex();
            if(coordinate[vdsIndex] < min || coordinate[vdsIndex] >= max) {
                const std::string coordinate_str =
                    "(" +std::to_string(x) + "," + std::to_string(y) + ")";
                throw std::runtime_error(
                    "Coordinate " + coordinate_str + " is out of boundaries "+
                    "in dimension "+ std::to_string(vdsIndex)+ "."
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
        coords[i][inlineAxis.getVDSIndex()] = coordinate[0];
        coords[i][crosslineAxis.getVDSIndex()] = coordinate[1];
        //}
    }

    //Get volume trace
    // TODO: Verify that trace dimension is always 0
    const auto size = vdsAccessManager.GetVolumeTracesBufferSize(
        numberOfPoints,
        0, //Trace dimension
        0, //Level of detail level
        0 // Channel
    );

    std::unique_ptr< char[] > data(new char[size]());

    auto request = vdsAccessManager.RequestVolumeTraces(
                        (float*)data.get(),
                        size,
                        OpenVDS::Dimensions_012,
                        0, //Level of detail level
                        0, // Channel
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
