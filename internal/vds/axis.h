#ifndef AXIS_H
#define AXIS_H

#include <memory>
#include <string>

#include <OpenVDS/OpenVDS.h>

#include "boundingbox.h"
#include "vds.h"

// TODO: Split this file into three separate files

// TODO: Fix case style.
//       Member variable names and functions are now camel case.
//       Temporary variables are sometimes camel case and sometimes snake case.
//       Temporary functions are (often) snake case.

// TODO: We could try to derive Axis from OpenVDS::VolumeDataAxisDescriptor
class Axis {
    private:
    int vdsIndex;
    int apiIndex;
    CoordinateSystem coordinateSystem;
    OpenVDS::VolumeDataAxisDescriptor vdsAxisDescriptor;
    std::string apiName; // Can we get rid of this?

    public:
    Axis(
        const ApiAxisName apiAxisName,
        OpenVDS::VolumeDataLayout const * vdsLayout
    );
    int getNumberOfPoints() const;
    int getMin() const;
    int getMax() const;
    std::string getUnit() const; //TODO: Could we work with string_views in C++17?
    int getVdsIndex() const;
    int getApiIndex() const; //TODO: Can I get rid of this?

    std::string getVdsName() const; //TODO: Could we work with string_views in C++17?
    std::string getApiName() const; //TODO: Could we work with string_views in C++17?
    //std::string getName() const;

    CoordinateSystem getCoordinateSystem() const;
};

class VDSMetadataHandler {
    private:
    OpenVDS::ScopedVDSHandle vdsHandle;
    OpenVDS::VolumeDataLayout const * vdsLayout;

    public:
    VDSMetadataHandler( const std::string url, const std::string credentials );

    Axis getInline() const;
    Axis getCrossline() const;
    Axis getSample() const;

    BoundingBox getBoundingBox() const;
    std::string getFormat() const;
    std::string getCRS() const;

    Axis getAxis(const ApiAxisName axisName) const;

    //TODO: Probably should be private and/or friend of VDSDataHandler
    //      Could/should this be a shared pointer?
    //      I am unsure whether the responsibilities are clear from the names of
    //      of the classes.
    OpenVDS::ScopedVDSHandle& getVDSHandle();

    static OpenVDS::InterpolationMethod getInterpolation(
        InterpolationMethod interpolation);
};

// TODO: Should this be derived from the MetadataHandler? To some extend "Data"
// might be interpreted as more general than MetaData. We could also rename it
// VDSVolumeDataHandler to make the distinction more clear.
class VDSDataHandler {
    private:
    VDSMetadataHandler metadata;

    public:

    VDSDataHandler(const std::string url, const std::string credentials);

    requestdata getSlice(const ApiAxisName& axisName, const int lineNumber);

    requestdata getFence(
        const CoordinateSystem    coordinateSystem,
        float const *             coordinates,
        const size_t              numberOfPoints,
        const InterpolationMethod interpolationMethod
    );
};

#endif /* AXIS_H */
