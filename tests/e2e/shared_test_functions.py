import requests
import numpy as np
import os
import json
import urllib.parse
from utils.cloud import *

from requests_toolbelt.multipart import decoder

STORAGE_ACCOUNT_NAME = os.getenv("STORAGE_ACCOUNT_NAME")
STORAGE_ACCOUNT_KEY = os.getenv("STORAGE_ACCOUNT_KEY")
ENDPOINT = os.getenv("ENDPOINT", "http://localhost:8080").rstrip("/")
CONTAINER = "testdata"
VDS = "well_known/well_known_default"
STORAGE_ACCOUNT = f"https://{STORAGE_ACCOUNT_NAME}.blob.core.windows.net"
VDSURL = f"{STORAGE_ACCOUNT}/{CONTAINER}/{VDS}"

SAMPLES10_URL = f"{STORAGE_ACCOUNT}/{CONTAINER}/10_samples/10_samples_default"


def gen_default_sas():
    return generate_container_signature(STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)


def surface():
    return {
        "xinc": 7.2111,
        "yinc": 3.6056,
        "xori": 2,
        "yori": 0,
        "rotation": 33.69,
    }


def make_slice_request(vds=VDSURL, direction="inline", lineno=3, sas="sas"):
    return {
        "vds": vds,
        "direction": direction,
        "lineno": lineno,
        "sas": sas
    }


def make_fence_request(vds=VDSURL, coordinate_system="ij", coordinates=[[0, 0]], sas="sas"):
    return {
        "vds": vds,
        "coordinateSystem": coordinate_system,
        "coordinates": coordinates,
        "sas": sas
    }


def make_metadata_request(vds=VDSURL, sas="sas"):
    return {
        "vds": vds,
        "sas": sas
    }


def make_attributes_along_surface_request(
    vds=SAMPLES10_URL,
    surface=surface(),
    values=[[20]],
    above=8,
    below=8,
    attributes=["samplevalue"],
    sas="sas"
):
    regular_surface = {
        "values": values,
        "fillValue": -999.25,
    }
    regular_surface.update(surface)

    request = {
        "surface": regular_surface,
        "interpolation": "nearest",
        "vds": vds,
        "sas": sas,
        "above": above,
        "below": below,
        "attributes": attributes
    }
    return request


def make_attributes_between_surfaces_request(
    primaryValues=[[20]],
    secondaryValues=[[20]],
    vds=SAMPLES10_URL,
    surface=surface(),
    attributes=["max"],
    stepsize=8,
    sas="sas"
):
    fillValue = -999.25
    primary = {
        "values": primaryValues,
        "fillValue": fillValue
    }
    primary.update(surface)
    secondary = {
        "values": secondaryValues,
        "fillValue": fillValue
    }
    secondary.update(surface)
    request = {
        "primarySurface": primary,
        "secondarySurface": secondary,
        "interpolation": "nearest",
        "vds": vds,
        "sas": sas,
        "stepsize": stepsize,
        "attributes": attributes
    }
    request.update(surface)
    return request


def send_request(path, method, payload):
    if method == "get":
        json_payload = json.dumps(payload)
        encoded_payload = urllib.parse.quote(json_payload)
        data = requests.get(f'{ENDPOINT}/{path}?query={encoded_payload}')
    elif method == "post":
        data = requests.post(f'{ENDPOINT}/{path}', json=payload)
    else:
        raise ValueError(f'Unknown method {method}')
    return data


def request_slice(method, lineno, direction):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)

    payload = make_slice_request(VDSURL, direction, lineno, sas)
    rdata = send_request("slice", method, payload)
    rdata.raise_for_status()

    multipart_data = decoder.MultipartDecoder.from_response(rdata)
    assert len(multipart_data.parts) == 2
    metadata = json.loads(multipart_data.parts[0].content)
    data = multipart_data.parts[1].content

    data = np.ndarray(metadata['shape'], metadata['format'], data)
    return metadata, data


def request_fence(method, coordinates, coordinate_system):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)

    payload = make_fence_request(VDSURL, coordinate_system, coordinates, sas)
    rdata = send_request("fence", method, payload)
    rdata.raise_for_status()

    multipart_data = decoder.MultipartDecoder.from_response(rdata)
    assert len(multipart_data.parts) == 2
    metadata = json.loads(multipart_data.parts[0].content)
    data = multipart_data.parts[1].content

    data = np.ndarray(metadata['shape'], metadata['format'], data)
    return metadata, data


def request_metadata(method):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)

    payload = make_metadata_request(VDSURL, sas)
    rdata = send_request("metadata", method, payload)
    rdata.raise_for_status()

    return rdata.json()


def request_attributes_along_surface(method, values):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)

    payload = make_attributes_along_surface_request(values=values, sas=sas)
    rdata = send_request("attributes/surface/along", method, payload)
    rdata.raise_for_status()

    multipart_data = decoder.MultipartDecoder.from_response(rdata)
    assert len(multipart_data.parts) == 2
    metadata = json.loads(multipart_data.parts[0].content)
    data = multipart_data.parts[1].content

    data = np.ndarray(metadata['shape'], metadata['format'], data)
    return metadata, data


def request_attributes_between_surfaces(method, primary, secondary):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)

    payload = make_attributes_between_surfaces_request(
        primary, secondary, sas=sas)
    rdata = send_request("attributes/surface/between", method, payload)
    rdata.raise_for_status()

    multipart_data = decoder.MultipartDecoder.from_response(rdata)
    assert len(multipart_data.parts) == 2
    metadata = json.loads(multipart_data.parts[0].content)
    data = multipart_data.parts[1].content

    data = np.ndarray(metadata['shape'], metadata['format'], data)
    return metadata, data
