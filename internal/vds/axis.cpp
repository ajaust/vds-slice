#include "axis.h"

//#include <iostream>
//#include <list>
//#include <unordered_map>
#include <stdexcept>

#include <OpenVDS/KnownMetadata.h>

Axis::Axis(
        const ApiAxisName apiAxisName,
        OpenVDS::VolumeDataLayout const * vdsLayout
    ) {

    // Find axis in VDS that corresponds to request's axis name
    char const * axisNameToFind;
    switch (apiAxisName) {
        case I:
        case INLINE:
            this->apiIndex = 0;
            axisNameToFind = OpenVDS::KnownAxisNames::Inline();
            break;
        case J:
        case CROSSLINE:
            this->apiIndex = 1;
            axisNameToFind = OpenVDS::KnownAxisNames::Crossline();
            break;
        case K:
        case DEPTH:
        case TIME:
        case SAMPLE:
            this->apiIndex = 2;
            axisNameToFind = OpenVDS::KnownAxisNames::Sample();
            break;
        default: {
            throw std::runtime_error("Unhandled axis");
        }
    }

    // TODO: This should get a test
    this->vdsIndex = -1;
    for (int vdsAxisIdx = 0; vdsAxisIdx < 3; ++vdsAxisIdx) {
        const auto& vdsAxisDescriptor = vdsLayout->GetAxisDescriptor(vdsAxisIdx);

        const bool names_are_equal = strcmp(
                                        axisNameToFind,
                                        vdsAxisDescriptor.GetName()
                                      ) == 0;

        if ( names_are_equal ) {
            this->vdsIndex = vdsAxisIdx;
            break;
        }
    }
    if (this->vdsIndex == -1) {
        std::string msg = "Could not find axis named "
                          + std::string(axisNameToFind)
                          + " in VDS.";
        throw std::runtime_error(msg);
    }
    // Get VDS axis descriptor
    vdsAxisDescriptor = vdsLayout->GetAxisDescriptor(vdsIndex);

    // Find axis name defined by API
    switch (apiAxisName) {
        case I:
            this->apiName = OpenVDS::KnownAxisNames::I();
            break;
        case INLINE:
            this->apiName = OpenVDS::KnownAxisNames::Inline();
            break;
        case J:
            this->apiName = OpenVDS::KnownAxisNames::J();
            break;
        case CROSSLINE:
            this->apiName = OpenVDS::KnownAxisNames::Crossline();
            break;
        case K:
            this->apiName = OpenVDS::KnownAxisNames::K();
            break;
        case DEPTH:
            this->apiName = OpenVDS::KnownAxisNames::Depth();
            break;
        case TIME:
            this->apiName = OpenVDS::KnownAxisNames::Time();
            break;
        case SAMPLE:
            this->apiName = OpenVDS::KnownAxisNames::Sample();
            break;
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
            throw std::runtime_error("Unhandled coordinate system");
        }
    }


    //Validate expections here now
    if (this->coordinateSystem == CoordinateSystem::ANNOTATION) {
        auto transformer = OpenVDS::IJKCoordinateTransformer(vdsLayout);
        if (not transformer.AnnotationsDefined()) {
            throw std::runtime_error("VDS doesn't define annotations");
        }
    }

    // This is also verifirde above
    //if (this->coordinateSystem != CoordinateSystem::ANNOTATION and
    //    this->coordinateSystem != CoordinateSystem::INDEX ) {
    //    throw std::runtime_error("Unhandled coordinate system");
    //}

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

std::string Axis::getUnit() const {
    std::string unit = this->vdsAxisDescriptor.GetUnit();
    return unit;
}

int Axis::getVdsIndex() const {
    return this->vdsIndex;
}

int Axis::getApiIndex() const {
    return this->apiIndex;
}

std::string Axis::getVdsName() const {
//std::string Axis::getName() const {
    std::string name = this->vdsAxisDescriptor.GetName();
    return name;
}

std::string Axis::getApiName() const {
    return this->apiName;
}

CoordinateSystem Axis::getCoordinateSystem() const {
    return this->coordinateSystem;
}
