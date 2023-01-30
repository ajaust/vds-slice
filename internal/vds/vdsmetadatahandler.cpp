#include "vdsmetadatahandler.h"

#include <OpenVDS/KnownMetadata.h>

VDSMetadataHandler::VDSMetadataHandler(
    const std::string url,
    const std::string credentials
)  {
    OpenVDS::Error error;
    this->vdsHandle = std::make_shared<OpenVDS::ScopedVDSHandle>(
                          OpenVDS::Open(url, credentials, error)
                      );
    if(error.code != 0) {
        throw std::runtime_error("Could not open VDS: " + error.string);
    }

    auto accessManager = OpenVDS::GetAccessManager(*this->vdsHandle);
    this->vdsLayout = accessManager.GetVolumeDataLayout();

    //Verify assumptions
    constexpr int expectedDimensionality = 3;
    if (vdsLayout->GetDimensionality() != expectedDimensionality) {
        throw std::runtime_error(
            "Unsupported VDS, expected 3 dimensions, got " +
            std::to_string(this->vdsLayout->GetDimensionality())
        );
    }
}

// TODO: Could also have a member of Axis for inline direction and return copy
//       or reference
Axis VDSMetadataHandler::getInline() const {
    return Axis(ApiAxisName::INLINE, this->vdsLayout);
}
// TODO: Could also have a member of Axis for crossline direction and return
//       copy or reference
Axis VDSMetadataHandler::getCrossline() const {
    return Axis(ApiAxisName::CROSSLINE, this->vdsLayout);
}
// TODO: Could also have a member of Axis for sample direction and return copy
//       or reference
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

Axis VDSMetadataHandler::getAxis(const ApiAxisName axisName) const {
    // TODO: Do we need to check this here AND when creating the axis?
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
    return Axis( axisName, this->vdsLayout );
}

OpenVDS::VolumeDataFormat VDSMetadataHandler::getChannelFormat(
    const int channelIndex
){
    return this->vdsLayout->GetChannelFormat(channelIndex);
}


std::shared_ptr<OpenVDS::ScopedVDSHandle> VDSMetadataHandler::getVDSHandle() {
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
