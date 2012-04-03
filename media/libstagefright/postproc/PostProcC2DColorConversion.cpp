/*
 * Copyright (c) 2012, The Linux Foundation. All rights reserved.
 * Not a Contribution, Apache license notifications and license are retained
 * for attribution purposes only.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "PostProcC2DColorConversion"
#include <PostProcC2DColorConversion.h>

#define NUMBER_C2D_BUFFERS 6

namespace android {

PostProcC2DColorConversion::PostProcC2DColorConversion()
{
    POSTPROC_LOGV("%s:  start", __func__);
}

PostProcC2DColorConversion::~PostProcC2DColorConversion()
{
    POSTPROC_LOGV("%s:  start", __func__);

    mC2DConvertClose(mC2DCC);
    dlclose(mC2DCCLibHandle);
}

status_t PostProcC2DColorConversion::init(sp<MediaSource> source, sp<PostProcNativeWindow> nativeWindow, const sp<MetaData> &meta, char *name)
{
    POSTPROC_LOGV("%s:  start", __func__);

    status_t err = PostProc::init(source, nativeWindow, meta, LOG_TAG);
    if (err != OK) {
        POSTPROC_LOGE("PostProc2Dto3D Init failed in base class");
        return err;
    }

    initC2DLibrary();
    C2DBuffReq bufferReq;
    err = mC2DCC->getBuffReq(C2D_OUTPUT, &bufferReq);
    mBufferSize = bufferReq.size;
    mStride = bufferReq.stride;
    mSlice = bufferReq.sliceHeight;
    return OK;
}

status_t PostProcC2DColorConversion::postProcessBuffer(MediaBuffer* inputBuffer, MediaBuffer* outputBuffer)
{
    POSTPROC_LOGV("%s:  start", __func__);

    CHECK(inputBuffer);
    CHECK(outputBuffer);
    return convertUsingC2D(inputBuffer, outputBuffer);
}

status_t PostProcC2DColorConversion::setBufferInfo(const sp<MetaData> &meta)
{
    POSTPROC_LOGV("%s:  begin", __func__);

    mOutputFormat = new MetaData(*(meta.get()));
    int32_t width;
    int32_t height;
    int32_t format;

    CHECK(mOutputFormat->findInt32(kKeyWidth, &width));
    CHECK(mOutputFormat->findInt32(kKeyHeight, &height));
    CHECK(mOutputFormat->findInt32(kKeyColorFormat, &format));

    mWidth = width;
    mHeight = height;
    mDstFormat = format;
    mNumBuffers = NUMBER_C2D_BUFFERS;
    return OK;
}

bool PostProcC2DColorConversion::postProcessingPossible()
{
    POSTPROC_LOGV("%s:  begin", __func__);

    int32_t threeDFormat, interlacedFormat;
    if ((mSource->getFormat()->findInt32(kKey3D, &threeDFormat)) ||
            (mSource->getFormat()->findInt32(kKeyInterlaced, &interlacedFormat))) {
        POSTPROC_LOGV("Detected 3D or interlaced format\n");
        return false;
    }
    return true;
}

status_t PostProcC2DColorConversion::initC2DLibrary()
{
    POSTPROC_LOGV("%s:  start", __func__);
    int32_t srcWidth;
    int32_t srcHeight;

    CHECK(mSource->getFormat()->findInt32(kKeyWidth, &srcWidth));
    CHECK(mSource->getFormat()->findInt32(kKeyHeight, &srcHeight));

    mC2DCCLibHandle = dlopen("libc2dcolorconvert.so", RTLD_LAZY);
    mC2DConvertOpen = NULL;
    mC2DConvertClose = NULL;
    mC2DCC = NULL;

    if (mC2DCCLibHandle) {
        mC2DConvertOpen = (createC2DColorConverter_t *)dlsym(mC2DCCLibHandle,"createC2DColorConverter");
        mC2DConvertClose = (destroyC2DColorConverter_t *)dlsym(mC2DCCLibHandle,"destroyC2DColorConverter");
        if(mC2DConvertOpen != NULL && mC2DConvertClose != NULL) {
            POSTPROC_LOGV("Successfully acquired mConvert symbol");
            mC2DCC = mC2DConvertOpen(srcWidth, srcHeight, mWidth, mHeight, getC2DFormat(mSrcFormat), getC2DFormat(mDstFormat), 0);
        } else {
            POSTPROC_LOGE("Could not get the c2d lib function pointers\n");
            CHECK(0);
        }
    }
    else {
        POSTPROC_LOGE("Could not get c2d yuvconversion lib handle");
        CHECK(0);
    }
    return OK;
}

status_t PostProcC2DColorConversion::convertUsingC2D(MediaBuffer* inputBuffer, MediaBuffer* outputBuffer)
{
    DurationTimer dt;
    dt.start();

    void * srcLuma = NULL;
    void * dstLuma = NULL;
    int srcFd = 0;
    int dstFd = 0;
    status_t err = OK;
    if (inputBuffer->graphicBuffer() != 0) {
        private_handle_t *inputHandle =
        (private_handle_t *) inputBuffer->graphicBuffer()->getNativeBuffer()->handle;
        inputBuffer->graphicBuffer()->lock(GRALLOC_USAGE_SW_READ_OFTEN |
            GRALLOC_USAGE_SW_WRITE_OFTEN, &srcLuma);
        srcFd = inputHandle->fd;
    } else {
        post_proc_media_buffer_type * packet = (post_proc_media_buffer_type *)inputBuffer->data();
        native_handle_t * nh = const_cast<native_handle_t *>(packet->meta_handle);
        srcFd = nh->data[0];
        srcLuma = (void *)nh->data[4];
    }

    POSTPROC_LOGV("srcLuma = %p", srcLuma);
    POSTPROC_LOGV("SrcFd = %d\n", srcFd);

    ion_handle * dstHnd = NULL;
    size_t dstSize = 0;
    size_t dstOffset = 0;
    if (outputBuffer->graphicBuffer() != 0) {
        private_handle_t *outputHandle =
        (private_handle_t *) outputBuffer->graphicBuffer()->getNativeBuffer()->handle;
        outputBuffer->graphicBuffer()->lock(GRALLOC_USAGE_SW_READ_OFTEN |
            GRALLOC_USAGE_SW_WRITE_OFTEN, &dstLuma);
        dstFd = outputHandle->fd;
    } else {
        post_proc_media_buffer_type * packet = (post_proc_media_buffer_type *)outputBuffer->data();
        native_handle_t * nh = const_cast<native_handle_t *>(packet->meta_handle);
        dstFd = nh->data[0];
        dstOffset = nh->data[1];
        dstSize = nh->data[2];
        dstHnd = (ion_handle *)nh->data[3];
        dstLuma = (void *)nh->data[4];
    }

    POSTPROC_LOGV("dstLuma = %p", dstLuma);
    POSTPROC_LOGV("DstFd = %d\n", dstFd);

    err = mC2DCC->convertC2D(srcFd, srcLuma, dstFd, dstLuma);

    if (err != OK) {
        POSTPROC_LOGE("Conversion failed\n");
    }

    if (inputBuffer->graphicBuffer() != 0) {
        inputBuffer->graphicBuffer()->unlock();
    }

    if (outputBuffer->graphicBuffer() != 0) {
        outputBuffer->graphicBuffer()->unlock();
    } else {
        if (err == OK) {
        #ifdef USE_ION
            struct ion_flush_data iondata;
            iondata.handle = dstHnd;
            iondata.fd = dstFd;
            iondata.vaddr = dstLuma;
            iondata.offset = dstOffset;
            iondata.length = dstSize;
            err = ioctl(mIonFd, ION_IOC_INV_CACHES, &iondata);
        #endif
        }
    }

    dt.stop();
    POSTPROC_LOGV("tile to nv12 elapsed = %f ms", ((float)dt.durationUsecs()/(float)1000));
    return err;
}

ColorConvertFormat PostProcC2DColorConversion::getC2DFormat(size_t format)
{
    POSTPROC_LOGV("Format is :%x\n", format);
    switch (format) {
        case OMX_QCOM_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka:
            return YCbCr420Tile;
        case OMX_QCOM_COLOR_FormatYVU420SemiPlanar:
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
            return YCbCr420SP;
        default:
            POSTPROC_LOGE("Format not supported\n");
            CHECK(0);
    }
}

}
