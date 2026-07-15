//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Slava Monich et al
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "tgsplugin.h"

#include <QFileDevice>
#include <QFileInfo>

#include "utilities.h"

#define DEBUG_MODULE TgsIOHandler
#include "debuglog.h"
#define LOG_(x) LOG(qPrintable(fileName) << x)

const QByteArray TgsIOHandler::NAME("tgs");

TgsIOHandler::TgsIOHandler(QIODevice* device, const QByteArray& format) :
    frameRate(0.),
    frameCount(0),
    currentFrame(0)
{
    QFileDevice* file = qobject_cast<QFileDevice*>(device);
    if (file) {
        fileName = QFileInfo(file->fileName()).fileName();
    }
    setDevice(device);
    setFormat(format);
}

TgsIOHandler::~TgsIOHandler()
{
    if (currentRender.valid()) {
        currentRender.get();
    }
    LOG_("Done");
}

TgsIOHandler::ByteArray TgsIOHandler::uncompress() {
    const QByteArray zipped(device()->readAll());

    if (zipped.mid(0, 2) != Utilities::GZ_MAGIC) {
        // Not compressed
        LOG_("Not compressed");
        return ByteArray(zipped.constData(), zipped.length());
    }

    LOG_("Uncompressing");
    return Utilities::uncompress(zipped);
}

bool TgsIOHandler::load()
{
    if (!animation && device()) {
        ByteArray json(uncompress());
        if (json.size() > 0) {
            animation = rlottie::Animation::loadFromData(json, std::string(), std::string(), false);
            if (animation) {
                size_t width, height;
                animation->size(width, height);
                frameRate = animation->frameRate();
                frameCount = (int) animation->totalFrame();
                size = QSize(width, height);
                LOG_(size << frameCount << "frames," << frameRate << "fps");
                render(0); // Pre-render first frame
            }
        }
    }
    return animation != Q_NULLPTR;
}

void TgsIOHandler::finishRendering()
{
    if (currentRender.valid()) {
        currentRender.get();
        prevImage = currentImage;
        if (!currentFrame && !firstImage.isNull()) {
            LOG_("Rendered first frame");
            firstImage = currentImage;
        }
    } else {
        // Must be the first frame
        prevImage = currentImage;
    }
}

void TgsIOHandler::render(int frameIndex)
{
    currentFrame = frameIndex % frameCount;
    if (!currentFrame && !firstImage.isNull()) {
        // The first frame only gets rendered once
        currentImage = firstImage;
    } else {
        int width, height;
        if (!scaledSize.isEmpty()) {
            width = scaledSize.width();
            height = scaledSize.height();
        } else {
            width = size.width();
            height = size.height();
        }
        currentImage = QImage(width, height, QImage::Format_ARGB32_Premultiplied);
        currentRender = animation->render(currentFrame,
            rlottie::Surface((uint32_t*)currentImage.bits(),
                width, height, currentImage.bytesPerLine()));
    }
}

bool TgsIOHandler::read(QImage* out)
{
    if (load() && frameCount > 0) {
        // We must have the first frame, will wait if necessary
        if (currentFrame && currentRender.valid()) {
            std::future_status status = currentRender.wait_for(std::chrono::milliseconds(0));
            if (status != std::future_status::ready) {
                LOG_("Skipping frame" << currentFrame);
                currentFrame = (currentFrame + 1) % frameCount;
                *out = prevImage;
                return true;
            }
        }
        finishRendering();
        *out = currentImage;
        render(currentFrame + 1);
        return true;
    }
    return false;
}

bool TgsIOHandler::canRead() const {
    return device();
}

QVariant TgsIOHandler::option(ImageOption option) const {
    switch (option) {
    case Size:
        ((TgsIOHandler*)this)->load(); // Cast off const
        return size;
    case Animation:
        return true;
    case ImageFormat:
        return QImage::Format_ARGB32_Premultiplied;
    default:
        break;
    }
    return QVariant();
}

bool TgsIOHandler::supportsOption(ImageOption option) const
{
    switch(option) {
    case Size:
    case Animation:
    case ImageFormat:
    case ScaledSize:
        return true;
    default:
        break;
    }
    return false;
}

void TgsIOHandler::setOption(ImageOption option, const QVariant &value) {
    switch(option) {
    case ScaledSize:
        if (scaledSize != value.toSize()) {
            scaledSize = value.toSize();
            LOG_("Scaled to" << scaledSize);
        }
        break;
    default:
        break;
    }
}

bool TgsIOHandler::jumpToNextImage()
{
    if (frameCount) {
        finishRendering();
        render(currentFrame + 1);
        return true;
    }
    return false;
}

bool TgsIOHandler::jumpToImage(int imageNumber)
{
    if (frameCount) {
        if (imageNumber != currentFrame) {
            finishRendering();
            render(imageNumber);
        }
        return true;
    }
    return false;
}

int TgsIOHandler::loopCount() const
{
    return -1;
}

int TgsIOHandler::imageCount() const
{
    return frameCount;
}

int TgsIOHandler::currentImageNumber() const
{
    return currentFrame;
}

QRect TgsIOHandler::currentImageRect() const
{
    return QRect(QPoint(), size);
}

int TgsIOHandler::nextImageDelay() const
{
    return frameRate > 0 ? (int)(1000/frameRate) : 33;
}

bool TgsIOHandler::currentRenderReady() const {
    if (frameCount && currentFrame && currentRender.valid()) {
        std::future_status status = currentRender.wait_for(std::chrono::milliseconds(0));
        return status == std::future_status::ready;
    }
    return false;
}

QImageIOPlugin::Capabilities TgsIOPlugin::capabilities(QIODevice*, const QByteArray& format) const {
    return Capabilities((format == TgsIOHandler::NAME) ? CanRead : 0);
}

QImageIOHandler* TgsIOPlugin::create(QIODevice* device, const QByteArray& format) const {
    return new TgsIOHandler(device, format);
}
