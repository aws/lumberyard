class TYPES:
    Byte = 1
    Ascii = 2
    Short = 3
    Long = 4
    Rational = 5
    SByte = 6
    Undefined = 7
    SShort = 8
    SLong = 9
    SRational = 10
    Float = 11
    DFloat = 12


TAGS = {
    'Image': {11: {'name': 'ProcessingSoftware', 'type': TYPES.Ascii},
               254: {'name': 'NewSubfileType', 'type': TYPES.Long},
               255: {'name': 'SubfileType', 'type': TYPES.Short},
               256: {'name': 'ImageWidth', 'type': TYPES.Long},
               257: {'name': 'ImageLength', 'type': TYPES.Long},
               258: {'name': 'BitsPerSample', 'type': TYPES.Short},
               259: {'name': 'Compression', 'type': TYPES.Short},
               262: {'name': 'PhotometricInterpretation', 'type': TYPES.Short},
               263: {'name': 'Threshholding', 'type': TYPES.Short},
               264: {'name': 'CellWidth', 'type': TYPES.Short},
               265: {'name': 'CellLength', 'type': TYPES.Short},
               266: {'name': 'FillOrder', 'type': TYPES.Short},
               269: {'name': 'DocumentName', 'type': TYPES.Ascii},
               270: {'name': 'ImageDescription', 'type': TYPES.Ascii},
               271: {'name': 'Make', 'type': TYPES.Ascii},
               272: {'name': 'Model', 'type': TYPES.Ascii},
               273: {'name': 'StripOffsets', 'type': TYPES.Long},
               274: {'name': 'Orientation', 'type': TYPES.Short},
               277: {'name': 'SamplesPerPixel', 'type': TYPES.Short},
               278: {'name': 'RowsPerStrip', 'type': TYPES.Long},
               279: {'name': 'StripByteCounts', 'type': TYPES.Long},
               282: {'name': 'XResolution', 'type': TYPES.Rational},
               283: {'name': 'YResolution', 'type': TYPES.Rational},
               284: {'name': 'PlanarConfiguration', 'type': TYPES.Short},
               290: {'name': 'GrayResponseUnit', 'type': TYPES.Short},
               291: {'name': 'GrayResponseCurve', 'type': TYPES.Short},
               292: {'name': 'T4Options', 'type': TYPES.Long},
               293: {'name': 'T6Options', 'type': TYPES.Long},
               296: {'name': 'ResolutionUnit', 'type': TYPES.Short},
               301: {'name': 'TransferFunction', 'type': TYPES.Short},
               305: {'name': 'Software', 'type': TYPES.Ascii},
               306: {'name': 'DateTime', 'type': TYPES.Ascii},
               315: {'name': 'Artist', 'type': TYPES.Ascii},
               316: {'name': 'HostComputer', 'type': TYPES.Ascii},
               317: {'name': 'Predictor', 'type': TYPES.Short},
               318: {'name': 'WhitePoint', 'type': TYPES.Rational},
               319: {'name': 'PrimaryChromaticities', 'type': TYPES.Rational},
               320: {'name': 'ColorMap', 'type': TYPES.Short},
               321: {'name': 'HalftoneHints', 'type': TYPES.Short},
               322: {'name': 'TileWidth', 'type': TYPES.Short},
               323: {'name': 'TileLength', 'type': TYPES.Short},
               324: {'name': 'TileOffsets', 'type': TYPES.Short},
               325: {'name': 'TileByteCounts', 'type': TYPES.Short},
               330: {'name': 'SubIFDs', 'type': TYPES.Long},
               332: {'name': 'InkSet', 'type': TYPES.Short},
               333: {'name': 'InkNames', 'type': TYPES.Ascii},
               334: {'name': 'NumberOfInks', 'type': TYPES.Short},
               336: {'name': 'DotRange', 'type': TYPES.Byte},
               337: {'name': 'TargetPrinter', 'type': TYPES.Ascii},
               338: {'name': 'ExtraSamples', 'type': TYPES.Short},
               339: {'name': 'SampleFormat', 'type': TYPES.Short},
               340: {'name': 'SMinSampleValue', 'type': TYPES.Short},
               341: {'name': 'SMaxSampleValue', 'type': TYPES.Short},
               342: {'name': 'TransferRange', 'type': TYPES.Short},
               343: {'name': 'ClipPath', 'type': TYPES.Byte},
               344: {'name': 'XClipPathUnits', 'type': TYPES.Long},
               345: {'name': 'YClipPathUnits', 'type': TYPES.Long},
               346: {'name': 'Indexed', 'type': TYPES.Short},
               347: {'name': 'JPEGTables', 'type': TYPES.Undefined},
               351: {'name': 'OPIProxy', 'type': TYPES.Short},
               512: {'name': 'JPEGProc', 'type': TYPES.Long},
               513: {'name': 'JPEGInterchangeFormat', 'type': TYPES.Long},
               514: {'name': 'JPEGInterchangeFormatLength', 'type': TYPES.Long},
               515: {'name': 'JPEGRestartInterval', 'type': TYPES.Short},
               517: {'name': 'JPEGLosslessPredictors', 'type': TYPES.Short},
               518: {'name': 'JPEGPointTransforms', 'type': TYPES.Short},
               519: {'name': 'JPEGQTables', 'type': TYPES.Long},
               520: {'name': 'JPEGDCTables', 'type': TYPES.Long},
               521: {'name': 'JPEGACTables', 'type': TYPES.Long},
               529: {'name': 'YCbCrCoefficients', 'type': TYPES.Rational},
               530: {'name': 'YCbCrSubSampling', 'type': TYPES.Short},
               531: {'name': 'YCbCrPositioning', 'type': TYPES.Short},
               532: {'name': 'ReferenceBlackWhite', 'type': TYPES.Rational},
               700: {'name': 'XMLPacket', 'type': TYPES.Byte},
               18246: {'name': 'Rating', 'type': TYPES.Short},
               18249: {'name': 'RatingPercent', 'type': TYPES.Short},
               32781: {'name': 'ImageID', 'type': TYPES.Ascii},
               33421: {'name': 'CFARepeatPatternDim', 'type': TYPES.Short},
               33422: {'name': 'CFAPattern', 'type': TYPES.Byte},
               33423: {'name': 'BatteryLevel', 'type': TYPES.Rational},
               33432: {'name': 'Copyright', 'type': TYPES.Ascii},
               33434: {'name': 'ExposureTime', 'type': TYPES.Rational},
               34377: {'name': 'ImageResources', 'type': TYPES.Byte},
               34665: {'name': 'ExifTag', 'type': TYPES.Long},
               34675: {'name': 'InterColorProfile', 'type': TYPES.Undefined},
               34853: {'name': 'GPSTag', 'type': TYPES.Long},
               34857: {'name': 'Interlace', 'type': TYPES.Short},
               34858: {'name': 'TimeZoneOffset', 'type': TYPES.Long},
               34859: {'name': 'SelfTimerMode', 'type': TYPES.Short},
               37387: {'name': 'FlashEnergy', 'type': TYPES.Rational},
               37388: {'name': 'SpatialFrequencyResponse', 'type': TYPES.Undefined},
               37389: {'name': 'Noise', 'type': TYPES.Undefined},
               37390: {'name': 'FocalPlaneXResolution', 'type': TYPES.Rational},
               37391: {'name': 'FocalPlaneYResolution', 'type': TYPES.Rational},
               37392: {'name': 'FocalPlaneResolutionUnit', 'type': TYPES.Short},
               37393: {'name': 'ImageNumber', 'type': TYPES.Long},
               37394: {'name': 'SecurityClassification', 'type': TYPES.Ascii},
               37395: {'name': 'ImageHistory', 'type': TYPES.Ascii},
               37397: {'name': 'ExposureIndex', 'type': TYPES.Rational},
               37398: {'name': 'TIFFEPStandardID', 'type': TYPES.Byte},
               37399: {'name': 'SensingMethod', 'type': TYPES.Short},
               40091: {'name': 'XPTitle', 'type': TYPES.Byte},
               40092: {'name': 'XPComment', 'type': TYPES.Byte},
               40093: {'name': 'XPAuthor', 'type': TYPES.Byte},
               40094: {'name': 'XPKeywords', 'type': TYPES.Byte},
               40095: {'name': 'XPSubject', 'type': TYPES.Byte},
               50341: {'name': 'PrintImageMatching', 'type': TYPES.Undefined},
               50706: {'name': 'DNGVersion', 'type': TYPES.Byte},
               50707: {'name': 'DNGBackwardVersion', 'type': TYPES.Byte},
               50708: {'name': 'UniqueCameraModel', 'type': TYPES.Ascii},
               50709: {'name': 'LocalizedCameraModel', 'type': TYPES.Byte},
               50710: {'name': 'CFAPlaneColor', 'type': TYPES.Byte},
               50711: {'name': 'CFALayout', 'type': TYPES.Short},
               50712: {'name': 'LinearizationTable', 'type': TYPES.Short},
               50713: {'name': 'BlackLevelRepeatDim', 'type': TYPES.Short},
               50714: {'name': 'BlackLevel', 'type': TYPES.Rational},
               50715: {'name': 'BlackLevelDeltaH', 'type': TYPES.SRational},
               50716: {'name': 'BlackLevelDeltaV', 'type': TYPES.SRational},
               50717: {'name': 'WhiteLevel', 'type': TYPES.Short},
               50718: {'name': 'DefaultScale', 'type': TYPES.Rational},
               50719: {'name': 'DefaultCropOrigin', 'type': TYPES.Short},
               50720: {'name': 'DefaultCropSize', 'type': TYPES.Short},
               50721: {'name': 'ColorMatrix1', 'type': TYPES.SRational},
               50722: {'name': 'ColorMatrix2', 'type': TYPES.SRational},
               50723: {'name': 'CameraCalibration1', 'type': TYPES.SRational},
               50724: {'name': 'CameraCalibration2', 'type': TYPES.SRational},
               50725: {'name': 'ReductionMatrix1', 'type': TYPES.SRational},
               50726: {'name': 'ReductionMatrix2', 'type': TYPES.SRational},
               50727: {'name': 'AnalogBalance', 'type': TYPES.Rational},
               50728: {'name': 'AsShotNeutral', 'type': TYPES.Short},
               50729: {'name': 'AsShotWhiteXY', 'type': TYPES.Rational},
               50730: {'name': 'BaselineExposure', 'type': TYPES.SRational},
               50731: {'name': 'BaselineNoise', 'type': TYPES.Rational},
               50732: {'name': 'BaselineSharpness', 'type': TYPES.Rational},
               50733: {'name': 'BayerGreenSplit', 'type': TYPES.Long},
               50734: {'name': 'LinearResponseLimit', 'type': TYPES.Rational},
               50735: {'name': 'CameraSerialNumber', 'type': TYPES.Ascii},
               50736: {'name': 'LensInfo', 'type': TYPES.Rational},
               50737: {'name': 'ChromaBlurRadius', 'type': TYPES.Rational},
               50738: {'name': 'AntiAliasStrength', 'type': TYPES.Rational},
               50739: {'name': 'ShadowScale', 'type': TYPES.SRational},
               50740: {'name': 'DNGPrivateData', 'type': TYPES.Byte},
               50741: {'name': 'MakerNoteSafety', 'type': TYPES.Short},
               50778: {'name': 'CalibrationIlluminant1', 'type': TYPES.Short},
               50779: {'name': 'CalibrationIlluminant2', 'type': TYPES.Short},
               50780: {'name': 'BestQualityScale', 'type': TYPES.Rational},
               50781: {'name': 'RawDataUniqueID', 'type': TYPES.Byte},
               50827: {'name': 'OriginalRawFileName', 'type': TYPES.Byte},
               50828: {'name': 'OriginalRawFileData', 'type': TYPES.Undefined},
               50829: {'name': 'ActiveArea', 'type': TYPES.Short},
               50830: {'name': 'MaskedAreas', 'type': TYPES.Short},
               50831: {'name': 'AsShotICCProfile', 'type': TYPES.Undefined},
               50832: {'name': 'AsShotPreProfileMatrix', 'type': TYPES.SRational},
               50833: {'name': 'CurrentICCProfile', 'type': TYPES.Undefined},
               50834: {'name': 'CurrentPreProfileMatrix', 'type': TYPES.SRational},
               50879: {'name': 'ColorimetricReference', 'type': TYPES.Short},
               50931: {'name': 'CameraCalibrationSignature', 'type': TYPES.Byte},
               50932: {'name': 'ProfileCalibrationSignature', 'type': TYPES.Byte},
               50934: {'name': 'AsShotProfileName', 'type': TYPES.Byte},
               50935: {'name': 'NoiseReductionApplied', 'type': TYPES.Rational},
               50936: {'name': 'ProfileName', 'type': TYPES.Byte},
               50937: {'name': 'ProfileHueSatMapDims', 'type': TYPES.Long},
               50938: {'name': 'ProfileHueSatMapData1', 'type': TYPES.Float},
               50939: {'name': 'ProfileHueSatMapData2', 'type': TYPES.Float},
               50940: {'name': 'ProfileToneCurve', 'type': TYPES.Float},
               50941: {'name': 'ProfileEmbedPolicy', 'type': TYPES.Long},
               50942: {'name': 'ProfileCopyright', 'type': TYPES.Byte},
               50964: {'name': 'ForwardMatrix1', 'type': TYPES.SRational},
               50965: {'name': 'ForwardMatrix2', 'type': TYPES.SRational},
               50966: {'name': 'PreviewApplicationName', 'type': TYPES.Byte},
               50967: {'name': 'PreviewApplicationVersion', 'type': TYPES.Byte},
               50968: {'name': 'PreviewSettingsName', 'type': TYPES.Byte},
               50969: {'name': 'PreviewSettingsDigest', 'type': TYPES.Byte},
               50970: {'name': 'PreviewColorSpace', 'type': TYPES.Long},
               50971: {'name': 'PreviewDateTime', 'type': TYPES.Ascii},
               50972: {'name': 'RawImageDigest', 'type': TYPES.Undefined},
               50973: {'name': 'OriginalRawFileDigest', 'type': TYPES.Undefined},
               50974: {'name': 'SubTileBlockSize', 'type': TYPES.Long},
               50975: {'name': 'RowInterleaveFactor', 'type': TYPES.Long},
               50981: {'name': 'ProfileLookTableDims', 'type': TYPES.Long},
               50982: {'name': 'ProfileLookTableData', 'type': TYPES.Float},
               51008: {'name': 'OpcodeList1', 'type': TYPES.Undefined},
               51009: {'name': 'OpcodeList2', 'type': TYPES.Undefined},
               51022: {'name': 'OpcodeList3', 'type': TYPES.Undefined},
               60606: {'name': 'ZZZTestSlong1', 'type': TYPES.SLong},
               60607: {'name': 'ZZZTestSlong2', 'type': TYPES.SLong},
               60608: {'name': 'ZZZTestSByte', 'type': TYPES.SByte},
               60609: {'name': 'ZZZTestSShort', 'type': TYPES.SShort},
               60610: {'name': 'ZZZTestDFloat', 'type': TYPES.DFloat},},
    'Exif': {33434: {'name': 'ExposureTime', 'type': TYPES.Rational},
             33437: {'name': 'FNumber', 'type': TYPES.Rational},
             34850: {'name': 'ExposureProgram', 'type': TYPES.Short},
             34852: {'name': 'SpectralSensitivity', 'type': TYPES.Ascii},
             34855: {'name': 'ISOSpeedRatings', 'type': TYPES.Short},
             34856: {'name': 'OECF', 'type': TYPES.Undefined},
             34864: {'name': 'SensitivityType', 'type': TYPES.Short},
             34865: {'name': 'StandardOutputSensitivity', 'type': TYPES.Long},
             34866: {'name': 'RecommendedExposureIndex', 'type': TYPES.Long},
             34867: {'name': 'ISOSpeed', 'type': TYPES.Long},
             34868: {'name': 'ISOSpeedLatitudeyyy', 'type': TYPES.Long},
             34869: {'name': 'ISOSpeedLatitudezzz', 'type': TYPES.Long},
             36864: {'name': 'ExifVersion', 'type': TYPES.Undefined},
             36867: {'name': 'DateTimeOriginal', 'type': TYPES.Ascii},
             36868: {'name': 'DateTimeDigitized', 'type': TYPES.Ascii},
             36880: {'name': 'OffsetTime', 'type': TYPES.Ascii},
             36881: {'name': 'OffsetTimeOriginal', 'type': TYPES.Ascii},
             36882: {'name': 'OffsetTimeDigitized', 'type': TYPES.Ascii},
             37121: {'name': 'ComponentsConfiguration', 'type': TYPES.Undefined},
             37122: {'name': 'CompressedBitsPerPixel', 'type': TYPES.Rational},
             37377: {'name': 'ShutterSpeedValue', 'type': TYPES.SRational},
             37378: {'name': 'ApertureValue', 'type': TYPES.Rational},
             37379: {'name': 'BrightnessValue', 'type': TYPES.SRational},
             37380: {'name': 'ExposureBiasValue', 'type': TYPES.SRational},
             37381: {'name': 'MaxApertureValue', 'type': TYPES.Rational},
             37382: {'name': 'SubjectDistance', 'type': TYPES.Rational},
             37383: {'name': 'MeteringMode', 'type': TYPES.Short},
             37384: {'name': 'LightSource', 'type': TYPES.Short},
             37385: {'name': 'Flash', 'type': TYPES.Short},
             37386: {'name': 'FocalLength', 'type': TYPES.Rational},
             37396: {'name': 'SubjectArea', 'type': TYPES.Short},
             37500: {'name': 'MakerNote', 'type': TYPES.Undefined},
             37510: {'name': 'UserComment', 'type': TYPES.Undefined},
             37520: {'name': 'SubSecTime', 'type': TYPES.Ascii},
             37521: {'name': 'SubSecTimeOriginal', 'type': TYPES.Ascii},
             37522: {'name': 'SubSecTimeDigitized', 'type': TYPES.Ascii},
             37888: {'name': 'Temperature', 'type': TYPES.SRational},
             37889: {'name': 'Humidity', 'type': TYPES.Rational},
             37890: {'name': 'Pressure', 'type': TYPES.Rational},
             37891: {'name': 'WaterDepth', 'type': TYPES.SRational},
             37892: {'name': 'Acceleration', 'type': TYPES.Rational},
             37893: {'name': 'CameraElevationAngle', 'type': TYPES.SRational},
             40960: {'name': 'FlashpixVersion', 'type': TYPES.Undefined},
             40961: {'name': 'ColorSpace', 'type': TYPES.Short},
             40962: {'name': 'PixelXDimension', 'type': TYPES.Long},
             40963: {'name': 'PixelYDimension', 'type': TYPES.Long},
             40964: {'name': 'RelatedSoundFile', 'type': TYPES.Ascii},
             40965: {'name': 'InteroperabilityTag', 'type': TYPES.Long},
             41483: {'name': 'FlashEnergy', 'type': TYPES.Rational},
             41484: {'name': 'SpatialFrequencyResponse', 'type': TYPES.Undefined},
             41486: {'name': 'FocalPlaneXResolution', 'type': TYPES.Rational},
             41487: {'name': 'FocalPlaneYResolution', 'type': TYPES.Rational},
             41488: {'name': 'FocalPlaneResolutionUnit', 'type': TYPES.Short},
             41492: {'name': 'SubjectLocation', 'type': TYPES.Short},
             41493: {'name': 'ExposureIndex', 'type': TYPES.Rational},
             41495: {'name': 'SensingMethod', 'type': TYPES.Short},
             41728: {'name': 'FileSource', 'type': TYPES.Undefined},
             41729: {'name': 'SceneType', 'type': TYPES.Undefined},
             41730: {'name': 'CFAPattern', 'type': TYPES.Undefined},
             41985: {'name': 'CustomRendered', 'type': TYPES.Short},
             41986: {'name': 'ExposureMode', 'type': TYPES.Short},
             41987: {'name': 'WhiteBalance', 'type': TYPES.Short},
             41988: {'name': 'DigitalZoomRatio', 'type': TYPES.Rational},
             41989: {'name': 'FocalLengthIn35mmFilm', 'type': TYPES.Short},
             41990: {'name': 'SceneCaptureType', 'type': TYPES.Short},
             41991: {'name': 'GainControl', 'type': TYPES.Short},
             41992: {'name': 'Contrast', 'type': TYPES.Short},
             41993: {'name': 'Saturation', 'type': TYPES.Short},
             41994: {'name': 'Sharpness', 'type': TYPES.Short},
             41995: {'name': 'DeviceSettingDescription', 'type': TYPES.Undefined},
             41996: {'name': 'SubjectDistanceRange', 'type': TYPES.Short},
             42016: {'name': 'ImageUniqueID', 'type': TYPES.Ascii},
             42032: {'name': 'CameraOwnerName', 'type': TYPES.Ascii},
             42033: {'name': 'BodySerialNumber', 'type': TYPES.Ascii},
             42034: {'name': 'LensSpecification', 'type': TYPES.Rational},
             42035: {'name': 'LensMake', 'type': TYPES.Ascii},
             42036: {'name': 'LensModel', 'type': TYPES.Ascii},
             42037: {'name': 'LensSerialNumber', 'type': TYPES.Ascii},
             42240: {'name': 'Gamma', 'type': TYPES.Rational}},
    'GPS': {0: {'name': 'GPSVersionID', 'type': TYPES.Byte},
                1: {'name': 'GPSLatitudeRef', 'type': TYPES.Ascii},
                2: {'name': 'GPSLatitude', 'type': TYPES.Rational},
                3: {'name': 'GPSLongitudeRef', 'type': TYPES.Ascii},
                4: {'name': 'GPSLongitude', 'type': TYPES.Rational},
                5: {'name': 'GPSAltitudeRef', 'type': TYPES.Byte},
                6: {'name': 'GPSAltitude', 'type': TYPES.Rational},
                7: {'name': 'GPSTimeStamp', 'type': TYPES.Rational},
                8: {'name': 'GPSSatellites', 'type': TYPES.Ascii},
                9: {'name': 'GPSStatus', 'type': TYPES.Ascii},
                10: {'name': 'GPSMeasureMode', 'type': TYPES.Ascii},
                11: {'name': 'GPSDOP', 'type': TYPES.Rational},
                12: {'name': 'GPSSpeedRef', 'type': TYPES.Ascii},
                13: {'name': 'GPSSpeed', 'type': TYPES.Rational},
                14: {'name': 'GPSTrackRef', 'type': TYPES.Ascii},
                15: {'name': 'GPSTrack', 'type': TYPES.Rational},
                16: {'name': 'GPSImgDirectionRef', 'type': TYPES.Ascii},
                17: {'name': 'GPSImgDirection', 'type': TYPES.Rational},
                18: {'name': 'GPSMapDatum', 'type': TYPES.Ascii},
                19: {'name': 'GPSDestLatitudeRef', 'type': TYPES.Ascii},
                20: {'name': 'GPSDestLatitude', 'type': TYPES.Rational},
                21: {'name': 'GPSDestLongitudeRef', 'type': TYPES.Ascii},
                22: {'name': 'GPSDestLongitude', 'type': TYPES.Rational},
                23: {'name': 'GPSDestBearingRef', 'type': TYPES.Ascii},
                24: {'name': 'GPSDestBearing', 'type': TYPES.Rational},
                25: {'name': 'GPSDestDistanceRef', 'type': TYPES.Ascii},
                26: {'name': 'GPSDestDistance', 'type': TYPES.Rational},
                27: {'name': 'GPSProcessingMethod', 'type': TYPES.Undefined},
                28: {'name': 'GPSAreaInformation', 'type': TYPES.Undefined},
                29: {'name': 'GPSDateStamp', 'type': TYPES.Ascii},
                30: {'name': 'GPSDifferential', 'type': TYPES.Short},
                31: {'name': 'GPSHPositioningError', 'type': TYPES.Rational}},
    'Interop': {1: {'name': 'InteroperabilityIndex', 'type': TYPES.Ascii}},
}

TAGS["0th"] = TAGS["Image"]
TAGS["1st"] = TAGS["Image"]

class ImageIFD:
    """Exif tag number reference - 0th IFD"""
    ProcessingSoftware = 11
    NewSubfileType = 254
    SubfileType = 255
    ImageWidth = 256
    ImageLength = 257
    BitsPerSample = 258
    Compression = 259
    PhotometricInterpretation = 262
    Threshholding = 263
    CellWidth = 264
    CellLength = 265
    FillOrder = 266
    DocumentName = 269
    ImageDescription = 270
    Make = 271
    Model = 272
    StripOffsets = 273
    Orientation = 274
    SamplesPerPixel = 277
    RowsPerStrip = 278
    StripByteCounts = 279
    XResolution = 282
    YResolution = 283
    PlanarConfiguration = 284
    GrayResponseUnit = 290
    GrayResponseCurve = 291
    T4Options = 292
    T6Options = 293
    ResolutionUnit = 296
    TransferFunction = 301
    Software = 305
    DateTime = 306
    Artist = 315
    HostComputer = 316
    Predictor = 317
    WhitePoint = 318
    PrimaryChromaticities = 319
    ColorMap = 320
    HalftoneHints = 321
    TileWidth = 322
    TileLength = 323
    TileOffsets = 324
    TileByteCounts = 325
    SubIFDs = 330
    InkSet = 332
    InkNames = 333
    NumberOfInks = 334
    DotRange = 336
    TargetPrinter = 337
    ExtraSamples = 338
    SampleFormat = 339
    SMinSampleValue = 340
    SMaxSampleValue = 341
    TransferRange = 342
    ClipPath = 343
    XClipPathUnits = 344
    YClipPathUnits = 345
    Indexed = 346
    JPEGTables = 347
    OPIProxy = 351
    JPEGProc = 512
    JPEGInterchangeFormat = 513
    JPEGInterchangeFormatLength = 514
    JPEGRestartInterval = 515
    JPEGLosslessPredictors = 517
    JPEGPointTransforms = 518
    JPEGQTables = 519
    JPEGDCTables = 520
    JPEGACTables = 521
    YCbCrCoefficients = 529
    YCbCrSubSampling = 530
    YCbCrPositioning = 531
    ReferenceBlackWhite = 532
    XMLPacket = 700
    Rating = 18246
    RatingPercent = 18249
    ImageID = 32781
    CFARepeatPatternDim = 33421
    CFAPattern = 33422
    BatteryLevel = 33423
    Copyright = 33432
    ExposureTime = 33434
    ImageResources = 34377
    ExifTag = 34665
    InterColorProfile = 34675
    GPSTag = 34853
    Interlace = 34857
    TimeZoneOffset = 34858
    SelfTimerMode = 34859
    FlashEnergy = 37387
    SpatialFrequencyResponse = 37388
    Noise = 37389
    FocalPlaneXResolution = 37390
    FocalPlaneYResolution = 37391
    FocalPlaneResolutionUnit = 37392
    ImageNumber = 37393
    SecurityClassification = 37394
    ImageHistory = 37395
    ExposureIndex = 37397
    TIFFEPStandardID = 37398
    SensingMethod = 37399
    XPTitle = 40091
    XPComment = 40092
    XPAuthor = 40093
    XPKeywords = 40094
    XPSubject = 40095
    PrintImageMatching = 50341
    DNGVersion = 50706
    DNGBackwardVersion = 50707
    UniqueCameraModel = 50708
    LocalizedCameraModel = 50709
    CFAPlaneColor = 50710
    CFALayout = 50711
    LinearizationTable = 50712
    BlackLevelRepeatDim = 50713
    BlackLevel = 50714
    BlackLevelDeltaH = 50715
    BlackLevelDeltaV = 50716
    WhiteLevel = 50717
    DefaultScale = 50718
    DefaultCropOrigin = 50719
    DefaultCropSize = 50720
    ColorMatrix1 = 50721
    ColorMatrix2 = 50722
    CameraCalibration1 = 50723
    CameraCalibration2 = 50724
    ReductionMatrix1 = 50725
    ReductionMatrix2 = 50726
    AnalogBalance = 50727
    AsShotNeutral = 50728
    AsShotWhiteXY = 50729
    BaselineExposure = 50730
    BaselineNoise = 50731
    BaselineSharpness = 50732
    BayerGreenSplit = 50733
    LinearResponseLimit = 50734
    CameraSerialNumber = 50735
    LensInfo = 50736
    ChromaBlurRadius = 50737
    AntiAliasStrength = 50738
    ShadowScale = 50739
    DNGPrivateData = 50740
    MakerNoteSafety = 50741
    CalibrationIlluminant1 = 50778
    CalibrationIlluminant2 = 50779
    BestQualityScale = 50780
    RawDataUniqueID = 50781
    OriginalRawFileName = 50827
    OriginalRawFileData = 50828
    ActiveArea = 50829
    MaskedAreas = 50830
    AsShotICCProfile = 50831
    AsShotPreProfileMatrix = 50832
    CurrentICCProfile = 50833
    CurrentPreProfileMatrix = 50834
    ColorimetricReference = 50879
    CameraCalibrationSignature = 50931
    ProfileCalibrationSignature = 50932
    AsShotProfileName = 50934
    NoiseReductionApplied = 50935
    ProfileName = 50936
    ProfileHueSatMapDims = 50937
    ProfileHueSatMapData1 = 50938
    ProfileHueSatMapData2 = 50939
    ProfileToneCurve = 50940
    ProfileEmbedPolicy = 50941
    ProfileCopyright = 50942
    ForwardMatrix1 = 50964
    ForwardMatrix2 = 50965
    PreviewApplicationName = 50966
    PreviewApplicationVersion = 50967
    PreviewSettingsName = 50968
    PreviewSettingsDigest = 50969
    PreviewColorSpace = 50970
    PreviewDateTime = 50971
    RawImageDigest = 50972
    OriginalRawFileDigest = 50973
    SubTileBlockSize = 50974
    RowInterleaveFactor = 50975
    ProfileLookTableDims = 50981
    ProfileLookTableData = 50982
    OpcodeList1 = 51008
    OpcodeList2 = 51009
    OpcodeList3 = 51022
    NoiseProfile = 51041
    ZZZTestSlong1 = 60606
    ZZZTestSlong2 = 60607
    ZZZTestSByte = 60608
    ZZZTestSShort = 60609
    ZZZTestDFloat = 60610


class ExifIFD:
    """Exif tag number reference - Exif IFD"""
    ExposureTime = 33434
    FNumber = 33437
    ExposureProgram = 34850
    SpectralSensitivity = 34852
    ISOSpeedRatings = 34855
    OECF = 34856
    SensitivityType = 34864
    StandardOutputSensitivity = 34865
    RecommendedExposureIndex = 34866
    ISOSpeed = 34867
    ISOSpeedLatitudeyyy = 34868
    ISOSpeedLatitudezzz = 34869
    ExifVersion = 36864
    DateTimeOriginal = 36867
    DateTimeDigitized = 36868
    OffsetTime = 36880
    OffsetTimeOriginal = 36881
    OffsetTimeDigitized = 36882
    ComponentsConfiguration = 37121
    CompressedBitsPerPixel = 37122
    ShutterSpeedValue = 37377
    ApertureValue = 37378
    BrightnessValue = 37379
    ExposureBiasValue = 37380
    MaxApertureValue = 37381
    SubjectDistance = 37382
    MeteringMode = 37383
    LightSource = 37384
    Flash = 37385
    FocalLength = 37386
    Temperature = 37888
    Humidity = 37889
    Pressure = 37890
    WaterDepth = 37891
    Acceleration = 37892
    CameraElevationAngle = 37893
    SubjectArea = 37396
    MakerNote = 37500
    UserComment = 37510
    SubSecTime = 37520
    SubSecTimeOriginal = 37521
    SubSecTimeDigitized = 37522
    FlashpixVersion = 40960
    ColorSpace = 40961
    PixelXDimension = 40962
    PixelYDimension = 40963
    RelatedSoundFile = 40964
    InteroperabilityTag = 40965
    FlashEnergy = 41483
    SpatialFrequencyResponse = 41484
    FocalPlaneXResolution = 41486
    FocalPlaneYResolution = 41487
    FocalPlaneResolutionUnit = 41488
    SubjectLocation = 41492
    ExposureIndex = 41493
    SensingMethod = 41495
    FileSource = 41728
    SceneType = 41729
    CFAPattern = 41730
    CustomRendered = 41985
    ExposureMode = 41986
    WhiteBalance = 41987
    DigitalZoomRatio = 41988
    FocalLengthIn35mmFilm = 41989
    SceneCaptureType = 41990
    GainControl = 41991
    Contrast = 41992
    Saturation = 41993
    Sharpness = 41994
    DeviceSettingDescription = 41995
    SubjectDistanceRange = 41996
    ImageUniqueID = 42016
    CameraOwnerName = 42032
    BodySerialNumber = 42033
    LensSpecification = 42034
    LensMake = 42035
    LensModel = 42036
    LensSerialNumber = 42037
    Gamma = 42240


class GPSIFD:
    """Exif tag number reference - GPS IFD"""
    GPSVersionID = 0
    GPSLatitudeRef = 1
    GPSLatitude = 2
    GPSLongitudeRef = 3
    GPSLongitude = 4
    GPSAltitudeRef = 5
    GPSAltitude = 6
    GPSTimeStamp = 7
    GPSSatellites = 8
    GPSStatus = 9
    GPSMeasureMode = 10
    GPSDOP = 11
    GPSSpeedRef = 12
    GPSSpeed = 13
    GPSTrackRef = 14
    GPSTrack = 15
    GPSImgDirectionRef = 16
    GPSImgDirection = 17
    GPSMapDatum = 18
    GPSDestLatitudeRef = 19
    GPSDestLatitude = 20
    GPSDestLongitudeRef = 21
    GPSDestLongitude = 22
    GPSDestBearingRef = 23
    GPSDestBearing = 24
    GPSDestDistanceRef = 25
    GPSDestDistance = 26
    GPSProcessingMethod = 27
    GPSAreaInformation = 28
    GPSDateStamp = 29
    GPSDifferential = 30
    GPSHPositioningError = 31


class InteropIFD:
    """Exif tag number reference - Interoperability IFD"""
    InteroperabilityIndex = 1
