#ifndef SEISMICMAP_H
#define SEISMICMAP_H


/// @brief Mapping between API coordinate system and VDS coordinate system axis
////        indices base class
class SeismicAxisMap {

public:
    /// @brief Dimension index of "Inline" axis in VDS coordinate system
    virtual int iline()  const noexcept (true) = 0;
    /// @brief Dimension index of "Crossline" axis in VDS coordinate system
    virtual int xline()  const noexcept (true) = 0;
    /// @brief Dimension index of "Sample" axis in VDS coordinate system
    virtual int sample() const noexcept (true) = 0;
    /// @brief Optional: Dimension index of "Offset" axis in VDS coordinate
    ///        system
    ///
    /// This function could be used for future pre-stack implementations.
    virtual int offset() const = 0;
    /// @brief Maps a VDS voxel dimension to spatial (request) dimension
    virtual int dimension_from( const int voxel ) const = 0;
    /// @brief Maps a spatial (request) dimension to VDS voxel dimension
    virtual int voxel_from( const int dimension ) const = 0;

    virtual ~SeismicAxisMap() {}
};

/// @brief Mapping between API coordinate system and VDS coordinate system axis
///        indices for post stack data.
///
/// For post-stack data it is expected that one has a three dimensional
/// coordinate system and that the inline, crossline and sample direction are
/// present in the VDS dataset. In the setting of VDS it means that one expects
/// the annotation system to be defined.
class PostStackAxisMap : public SeismicAxisMap {
public:

    PostStackAxisMap(int i, int x, int s);

    int iline()  const noexcept (true) override final;
    int xline()  const noexcept (true) override final;
    int sample() const noexcept (true) override final;
    int offset() const override final;
    int dimension_from( const int voxel ) const override final;
    int voxel_from( const int dimension ) const override final;

private:
    /// @brief Axis id of inline dimension in OpenVDS' coordinate system
    const int inline_axis_id_;
    /// @brief Axis id of crossline dimension in OpenVDS' coordinate system
    const int crossline_axis_id_;
    /// @brief Axis id of sample dimension in OpenVDS' coordinate system
    const int sample_axis_id_;
};

#endif /* SEISMICMAP_H */
