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

#endif /* AXIS_H */
