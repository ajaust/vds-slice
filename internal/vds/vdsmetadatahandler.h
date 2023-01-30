#ifndef VDSMETADATAHANDLER_H
#define VDSMETADATAHANDLER_H

#include <string>

#include <OpenVDS/OpenVDS.h>

#include "axis.h"
#include "boundingbox.h"

class VDSMetadataHandler {
    private:
    OpenVDS::ScopedVDSHandle vdsHandle;
    OpenVDS::VolumeDataLayout const * vdsLayout;

    public:
    VDSMetadataHandler(const std::string url, const std::string credentials);

    Axis getInline()    const;
    Axis getCrossline() const;
    Axis getSample()    const;

    BoundingBox getBoundingBox() const;
    std::string getFormat()      const;
    std::string getCRS()         const;

    Axis getAxis(const ApiAxisName axisName) const;

    // TODO: This function limits exposure of vdsLayout pointer outside of this
    //       class.
    OpenVDS::VolumeDataFormat getChannelFormat(const int channelIndex);
    //TODO: Currently needed such that DataHandler can access the VDS dataset
    //      which is opened by VDSMetadataHandler.
    //      Probably should be private and/or friend of VDSDataHandler.
    //      Could/should this be a shared pointer?
    //      I am unsure whether the responsibilities are clear from the names of
    //      of the classes.
    OpenVDS::ScopedVDSHandle& getVDSHandle();

    static OpenVDS::InterpolationMethod getInterpolation(
        InterpolationMethod interpolation);
};


#endif /* VDSMETADATAHANDLER_H */
