package api

import (
	"encoding/json"
	"net/http"
	"strings"

	"github.com/gin-gonic/gin"
	"github.com/gin-gonic/gin/binding"

	"github.com/equinor/vds-slice/internal/vds"
)

type BaseRequest struct {
	// The blob url to a vds in form
	// https://account.blob.core.windows.net/container/blob
	Vds string `json:"vds" binding:"required" example:"https://account.blob.core.windows.net/container/blob"`
	// A valid sas-token with read access to the container specified in Vds
	Sas string `json:"sas" binding:"required" example:"sp=r&st=2022-09-12T09:44:17Z&se=2022-09-12T17:44:17Z&spr=https&sv=2021-06-08&sr=c&sig=..."`
}

type MetadataRequest struct {
	BaseRequest
} //@name MetadataRequest

type FenceRequest struct {
	BaseRequest
	// Coordinate system for the requested fence
	// Supported options are:
	// ilxl : inline, crossline pairs
	// ij   : Coordinates are given as in 0-indexed system, where the first
	//        line in each direction is 0 and the last is number-of-lines - 1.
	// cdp  : Coordinates are given as cdpx/cdpy pairs. In the original SEGY
	//        this would correspond to the cdpx and cdpy fields in the
	//        trace-headers after applying the scaling factor.
	CoordinateSystem string `json:"coordinateSystem" binding:"required" example:"cdp"`

	// A list of (x, y) points in the coordinate system specified in
	// coordinateSystem, for example [[2000.5, 100.5], [2050, 200], [10, 20]].
	// All coordinates are expected to be inside the bounding box (with outer margin
	// of half a distance between consecutive lines).
	Coordinates [][]float32 `json:"coordinates" binding:"required"`

	// Interpolation method
	// Supported options are: nearest, linear, cubic, angular and triangular.
	// Defaults to nearest.
	// This field is passed on to OpenVDS, which does the actual interpolation.
	// Note: For nearest interpolation result will snap to the nearest point
	// as per "half up" rounding. This is different from openvds logic.
	Interpolation string `json:"interpolation" example:"linear"`
} //@name FenceRequest

// Query for slice endpoints
// @Description Query payload for slice endpoint /slice.
type SliceRequest struct {
	BaseRequest

	// Direction can be specified in two domains
	// - Annotation. Valid options: Inline, Crossline and Depth/Time/Sample
	// - Index. Valid options: i, j and k.
	//
	// Only one of Depth, Time and Sample is valid for a given VDS. Which one
	// depends on the depth units. E.g. Depth is a valid option for a VDS with
	// depth unit "m" or "ft", but not if the units are "ms", "s".
	// Sample is valid when the depth is unitless.
	//
	// i, j, k are zero-indexed and correspond to Inline, Crossline,
	// Depth/Time/Sample, respectively.
	//
	// All options are case-insensitive.
	Direction string `json:"direction" binding:"required" example:"inline"`

	// Line number of the slice
	Lineno *int `json:"lineno" binding:"required" example:"10000"`
} //@name SliceRequest

type Endpoint struct {
	MakeVdsConnection vds.ConnectionMaker
}

func (e *Endpoint) metadata(ctx *gin.Context, query MetadataRequest) {
	conn, err := e.MakeVdsConnection(query.Vds, query.Sas)
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	buffer, err := vds.GetMetadata(*conn)
	if err != nil {
		ctx.AbortWithError(http.StatusInternalServerError, err)
		return
	}

	ctx.Data(http.StatusOK, "application/json", buffer)
}

func (e *Endpoint) slice(ctx *gin.Context, query SliceRequest) {
	conn, err := e.MakeVdsConnection(query.Vds, query.Sas)
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	axis, err := vds.GetAxis(strings.ToLower(query.Direction))
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	metadata, err := vds.GetSliceMetadata(*conn, *query.Lineno, axis)
	if err != nil {
		ctx.AbortWithError(http.StatusInternalServerError, err)
		return
	}

	data, err := vds.GetSlice(*conn, *query.Lineno, axis)
	if err != nil {
		ctx.AbortWithError(http.StatusInternalServerError, err)
		return
	}
	writeResponse(ctx, metadata, data)
}

func (e *Endpoint) fence(ctx *gin.Context, query FenceRequest) {
	conn, err := e.MakeVdsConnection(query.Vds, query.Sas)
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	coordinateSystem, err := vds.GetCoordinateSystem(strings.ToLower(query.CoordinateSystem))
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	interpolation, err := vds.GetInterpolationMethod(query.Interpolation)
	if err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}

	metadata, err := vds.GetFenceMetadata(*conn, query.Coordinates)
	if err != nil {
		ctx.AbortWithError(http.StatusInternalServerError, err)
		return
	}

	data, err := vds.GetFence(
		*conn,
		coordinateSystem,
		query.Coordinates,
		interpolation,
	)

	if err != nil {
		ctx.AbortWithError(http.StatusInternalServerError, err)
		return
	}

	writeResponse(ctx, metadata, data)
}

func parseGetRequest(ctx *gin.Context, v interface{}) error {
	if err := json.Unmarshal([]byte(ctx.Query("query")), v); err != nil {
		return err
	}

	return binding.Validator.ValidateStruct(v)
}

func (e *Endpoint) Health(ctx *gin.Context) {
	ctx.String(http.StatusOK, "I am up and running")
}

// MetadataGet godoc
// @Summary  Return volumetric metadata about the VDS
// @Tags     metadata
// @Param    query  query  string  True  "Urlencoded/escaped MetadataRequest"
// @Produce  json
// @Success  200 {object} Metadata
// @Failure  400 {object} ErrorResponse "Request is invalid"
// @Failure  500 {object} ErrorResponse "openvds failed to process the request"
// @Router   /metadata  [get]
func (e *Endpoint) MetadataGet(ctx *gin.Context) {
	var query MetadataRequest
	if err := parseGetRequest(ctx, &query); err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}
	e.metadata(ctx, query)
}

// MetadataPost godoc
// @Summary  Return volumetric metadata about the VDS
// @Tags     metadata
// @Param    body  body  MetadataRequest  True  "Request parameters"
// @Produce  json
// @Success  200 {object} Metadata
// @Failure  400 {object} ErrorResponse "Request is invalid"
// @Failure  500 {object} ErrorResponse "openvds failed to process the request"
// @Router   /metadata  [post]
func (e *Endpoint) MetadataPost(ctx *gin.Context) {
	var query MetadataRequest
	if err := ctx.ShouldBind(&query); err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}
	e.metadata(ctx, query)
}

// SliceGet godoc
// @Summary  Fetch a slice from a VDS
// @description.markdown slice
// @Tags     slice
// @Param    query  query  string  True  "Urlencoded/escaped SliceRequest"
// @Produce  multipart/mixed
// @Success  200 {object} SliceMetadata "(Example below only for metadata part)"
// @Failure  400 {object} ErrorResponse "Request is invalid"
// @Failure  500 {object} ErrorResponse "openvds failed to process the request"
// @Router   /slice  [get]
func (e *Endpoint) SliceGet(ctx *gin.Context) {
	var query SliceRequest
	if err := parseGetRequest(ctx, &query); err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}
	e.slice(ctx, query)
}

// SlicePost godoc
// @Summary  Fetch a slice from a VDS
// @description.markdown slice
// @Tags     slice
// @Param    body  body  SliceRequest  True  "Query Parameters"
// @Accept   application/json
// @Produce  multipart/mixed
// @Success  200 {object} SliceMetadata "(Example below only for metadata part)"
// @Failure  400 {object} ErrorResponse "Request is invalid"
// @Failure  500 {object} ErrorResponse "openvds failed to process the request"
// @Router   /slice  [post]
func (e *Endpoint) SlicePost(ctx *gin.Context) {
	var query SliceRequest
	if err := ctx.ShouldBind(&query); err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}
	e.slice(ctx, query)
}

// FenceGet godoc
// @Summary  Returns traces along an arbitrary path, such as a well-path
// @description.markdown fence
// @Tags     fence
// @Param    query  query  string  True  "Urlencoded/escaped FenceResponse"
// @Accept   application/json
// @Produce  multipart/mixed
// @Success  200 {object} FenceMetadata "(Example below only for metadata part)"
// @Failure  400 {object} ErrorResponse "Request is invalid"
// @Failure  500 {object} ErrorResponse "openvds failed to process the request"
// @Router   /fence  [get]
func (e *Endpoint) FenceGet(ctx *gin.Context) {
	var query FenceRequest
	if err := parseGetRequest(ctx, &query); err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}
	e.fence(ctx, query)
}

// FencePost godoc
// @Summary  Returns traces along an arbitrary path, such as a well-path
// @description.markdown fence
// @Tags     fence
// @Param    body  body  FenceRequest  True  "Request Parameters"
// @Accept   application/json
// @Produce  multipart/mixed
// @Success  200 {object} FenceMetadata "(Example below only for metadata part)"
// @Failure  400 {object} ErrorResponse "Request is invalid"
// @Failure  500 {object} ErrorResponse "openvds failed to process the request"
// @Router   /fence  [post]
func (e *Endpoint) FencePost(ctx *gin.Context) {
	var query FenceRequest
	if err := ctx.ShouldBind(&query); err != nil {
		ctx.AbortWithError(http.StatusBadRequest, err)
		return
	}
	e.fence(ctx, query)
}
