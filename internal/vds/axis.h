#ifndef AXIS_H
#define AXIS_H

#include <string>

#include <OpenVDS/OpenVDS.h>

#include "boundingbox.h"
#include "vds.h"

// TODO: We could try to derive Axis from OpenVDS::VolumeDataAxisDescriptor
class Axis {
    private:
    int vdsIndex;
    int apiIndex;
    CoordinateSystem coordinateSystem;
    OpenVDS::VolumeDataAxisDescriptor vdsAxisDescriptor;

    std::string apiName; // Can we get rid of this

    public:
    Axis(
        const ApiAxisName apiAxisName,
        const OpenVDS::VolumeDataLayout* layout
    );
    int getNumberOfPoints() const;
    int getMin() const;
    int getMax() const;
    std::string getName() const; //TODO: Could we work with string_views in C++17?
    std::string getUnit() const; //TODO: Could we work with string_views in C++17?
    int getVDSIndex() const;
    int getAPIIndex() const; //TODO: Can I get rid of this?

    std::string getAPIName() const;

    CoordinateSystem getCoordinateSystem() const;
};

class VDSMetadataHandler {
    private:
    OpenVDS::ScopedVDSHandle vdsHandle;
    OpenVDS::VolumeDataLayout const * vdsLayout;

    public:
    VDSMetadataHandler( const std::string url, const std::string credentials );
    //VDSMetadataHandler( OpenVDS::ScopedVDSHandle vdsHandle );

    Axis getInline() const;
    Axis getCrossline() const;
    Axis getSample() const;

    BoundingBox getBoundingBox() const;
    std::string getFormat() const;
    std::string getCRS() const;

    Axis getAxis(const ApiAxisName an);

    //TODO: Probably should be private and/or friend of VDSDataHandler
    OpenVDS::ScopedVDSHandle& getVDSHandle();

    static OpenVDS::InterpolationMethod getInterpolation(
        InterpolationMethod interpolation);
};

// TODO: Should this be derived from the MetadataHandler? To some extend "Data"
// might be interpreted as more general than MetaData. We could also rename it
// VDSVolumeDataHandler to make the distinction more clear.
class VDSDataHandler {
    VDSMetadataHandler metadata;

    public:

    VDSDataHandler(const std::string url, const std::string credentials);

    requestdata getSlice(const ApiAxisName& axisName, const int lineNumber);
    //void getFence()

    requestdata getFence(
        const CoordinateSystem    coordinateSystem,
        float const *             coordinates,
        const size_t              numberOfPoints,
        const InterpolationMethod interpolationMethod
    );
};



#endif /* AXIS_H */
