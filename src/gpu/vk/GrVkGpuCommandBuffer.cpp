/*
* Copyright 2016 Google Inc.
*
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*/

#include "GrVkGpuCommandBuffer.h"

#include "GrFixedClip.h"
#include "GrMesh.h"
#include "GrOpFlushState.h"
#include "GrPipeline.h"
#include "GrRenderTargetPriv.h"
#include "GrTexturePriv.h"
#include "GrVkCommandBuffer.h"
#include "GrVkGpu.h"
#include "GrVkPipeline.h"
#include "GrVkRenderPass.h"
#include "GrVkRenderTarget.h"
#include "GrVkResourceProvider.h"
#include "GrVkTexture.h"
#include "SkRect.h"

void GrVkGpuTextureCommandBuffer::copy(GrSurface* src, const SkIRect& srcRect,
                                       const SkIPoint& dstPoint) {
    fCopies.emplace_back(src, srcRect, dstPoint);
}

void GrVkGpuTextureCommandBuffer::insertEventMarker(const char* msg) {
    // TODO: does Vulkan have a correlate?
}

void GrVkGpuTextureCommandBuffer::submit() {
    for (int i = 0; i < fCopies.count(); ++i) {
        CopyInfo& copyInfo = fCopies[i];
        fGpu->copySurface(fTexture, copyInfo.fSrc, copyInfo.fSrcRect, copyInfo.fDstPoint);
    }
}

GrVkGpuTextureCommandBuffer::~GrVkGpuTextureCommandBuffer() {}

////////////////////////////////////////////////////////////////////////////////

void get_vk_load_store_ops(GrGpuRTCommandBuffer::LoadOp loadOpIn,
                           GrGpuRTCommandBuffer::StoreOp storeOpIn,
                           VkAttachmentLoadOp* loadOp, VkAttachmentStoreOp* storeOp) {
    switch (loadOpIn) {
        case GrGpuRTCommandBuffer::LoadOp::kLoad:
            *loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            break;
        case GrGpuRTCommandBuffer::LoadOp::kClear:
            *loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            break;
        case GrGpuRTCommandBuffer::LoadOp::kDiscard:
            *loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            break;
        default:
            SK_ABORT("Invalid LoadOp");
            *loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    }

    switch (storeOpIn) {
        case GrGpuRTCommandBuffer::StoreOp::kStore:
            *storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            break;
        case GrGpuRTCommandBuffer::StoreOp::kDiscard:
            *storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            break;
        default:
            SK_ABORT("Invalid StoreOp");
            *storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    }
}

GrVkGpuRTCommandBuffer::GrVkGpuRTCommandBuffer(GrVkGpu* gpu,
                                               GrRenderTarget* rt, GrSurfaceOrigin origin,
                                               const LoadAndStoreInfo& colorInfo,
                                               const StencilLoadAndStoreInfo& stencilInfo)
        : INHERITED(rt, origin)
        , fGpu(gpu)
        , fClearColor(GrColor4f::FromGrColor(colorInfo.fClearColor))
        , fLastPipelineState(nullptr) {
    get_vk_load_store_ops(colorInfo.fLoadOp, colorInfo.fStoreOp,
                          &fVkColorLoadOp, &fVkColorStoreOp);

    get_vk_load_store_ops(stencilInfo.fLoadOp, stencilInfo.fStoreOp,
                          &fVkStencilLoadOp, &fVkStencilStoreOp);
    fCurrentCmdInfo = -1;

    this->init();
}

void GrVkGpuRTCommandBuffer::init() {
    GrVkRenderPass::LoadStoreOps vkColorOps(fVkColorLoadOp, fVkColorStoreOp);
    GrVkRenderPass::LoadStoreOps vkStencilOps(fVkStencilLoadOp, fVkStencilStoreOp);

    CommandBufferInfo& cbInfo = fCommandBufferInfos.push_back();
    SkASSERT(fCommandBufferInfos.count() == 1);
    fCurrentCmdInfo = 0;

    GrVkRenderTarget* vkRT = static_cast<GrVkRenderTarget*>(fRenderTarget);
    const GrVkResourceProvider::CompatibleRPHandle& rpHandle = vkRT->compatibleRenderPassHandle();
    if (rpHandle.isValid()) {
        cbInfo.fRenderPass = fGpu->resourceProvider().findRenderPass(rpHandle,
                                                                     vkColorOps,
                                                                     vkStencilOps);
    } else {
        cbInfo.fRenderPass = fGpu->resourceProvider().findRenderPass(*vkRT,
                                                                     vkColorOps,
                                                                     vkStencilOps);
    }

    cbInfo.fColorClearValue.color.float32[0] = fClearColor.fRGBA[0];
    cbInfo.fColorClearValue.color.float32[1] = fClearColor.fRGBA[1];
    cbInfo.fColorClearValue.color.float32[2] = fClearColor.fRGBA[2];
    cbInfo.fColorClearValue.color.float32[3] = fClearColor.fRGBA[3];

    cbInfo.fBounds.setEmpty();
    cbInfo.fIsEmpty = true;
    cbInfo.fStartsWithClear = false;

    cbInfo.fCommandBuffers.push_back(fGpu->resourceProvider().findOrCreateSecondaryCommandBuffer());
    cbInfo.currentCmdBuf()->begin(fGpu, vkRT->framebuffer(), cbInfo.fRenderPass);
}


GrVkGpuRTCommandBuffer::~GrVkGpuRTCommandBuffer() {
    for (int i = 0; i < fCommandBufferInfos.count(); ++i) {
        CommandBufferInfo& cbInfo = fCommandBufferInfos[i];
        for (int j = 0; j < cbInfo.fCommandBuffers.count(); ++j) {
            cbInfo.fCommandBuffers[j]->unref(fGpu);
        }
        cbInfo.fRenderPass->unref(fGpu);
    }
}

GrGpu* GrVkGpuRTCommandBuffer::gpu() { return fGpu; }

void GrVkGpuRTCommandBuffer::end() {
    if (fCurrentCmdInfo >= 0) {
        fCommandBufferInfos[fCurrentCmdInfo].currentCmdBuf()->end(fGpu);
    }
}

void GrVkGpuRTCommandBuffer::submit() {
    if (!fRenderTarget) {
        return;
    }

    GrVkRenderTarget* vkRT = static_cast<GrVkRenderTarget*>(fRenderTarget);

    // Change layout of our render target so it can be used as the color attachment. Currently
    // we don't attach the resolve to the framebuffer so no need to change its layout.
    GrVkImage* targetImage = vkRT->msaaImage() ? vkRT->msaaImage() : vkRT;

    // Change layout of our render target so it can be used as the color attachment
    targetImage->setImageLayout(fGpu,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                                false);

    // If we are using a stencil attachment we also need to update its layout
    if (GrStencilAttachment* stencil = fRenderTarget->renderTargetPriv().getStencilAttachment()) {
        GrVkStencilAttachment* vkStencil = (GrVkStencilAttachment*)stencil;
        vkStencil->setImageLayout(fGpu,
                                  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                                  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                                  VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                                  false);
    }

    for (int i = 0; i < fCommandBufferInfos.count(); ++i) {
        CommandBufferInfo& cbInfo = fCommandBufferInfos[i];

        for (int j = 0; j < cbInfo.fPreDrawUploads.count(); ++j) {
            InlineUploadInfo& iuInfo = cbInfo.fPreDrawUploads[j];
            iuInfo.fFlushState->doUpload(iuInfo.fUpload);
        }

        for (int j = 0; j < cbInfo.fPreCopies.count(); ++j) {
            CopyInfo& copyInfo = cbInfo.fPreCopies[j];
            fGpu->copySurface(fRenderTarget, copyInfo.fSrc, copyInfo.fSrcRect, copyInfo.fDstPoint);
        }

        // TODO: We can't add this optimization yet since many things create a scratch texture which
        // adds the discard immediately, but then don't draw to it right away. This causes the
        // discard to be ignored and we get yelled at for loading uninitialized data. However, once
        // MDB lands, the discard will get reordered with the rest of the draw commands and we can
        // re-enable this.
#if 0
        if (cbInfo.fIsEmpty && !cbInfo.fStartsWithClear) {
            // We have sumbitted no actual draw commands to the command buffer and we are not using
            // the render pass to do a clear so there is no need to submit anything.
            continue;
        }
#endif
        if (cbInfo.fBounds.intersect(0, 0,
                                     SkIntToScalar(fRenderTarget->width()),
                                     SkIntToScalar(fRenderTarget->height()))) {
            SkIRect iBounds;
            cbInfo.fBounds.roundOut(&iBounds);

            fGpu->submitSecondaryCommandBuffer(cbInfo.fCommandBuffers, cbInfo.fRenderPass,
                                               &cbInfo.fColorClearValue, vkRT, iBounds);
        }
    }
}

void GrVkGpuRTCommandBuffer::discard() {
    GrVkRenderTarget* vkRT = static_cast<GrVkRenderTarget*>(fRenderTarget);

    CommandBufferInfo& cbInfo = fCommandBufferInfos[fCurrentCmdInfo];
    if (cbInfo.fIsEmpty) {
        // Change the render pass to do a don't-care load for both color & stencil
        GrVkRenderPass::LoadStoreOps vkColorOps(VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                                VK_ATTACHMENT_STORE_OP_STORE);
        GrVkRenderPass::LoadStoreOps vkStencilOps(VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                                  VK_ATTACHMENT_STORE_OP_STORE);

        const GrVkRenderPass* oldRP = cbInfo.fRenderPass;

        const GrVkResourceProvider::CompatibleRPHandle& rpHandle =
            vkRT->compatibleRenderPassHandle();
        if (rpHandle.isValid()) {
            cbInfo.fRenderPass = fGpu->resourceProvider().findRenderPass(rpHandle,
                                                                         vkColorOps,
                                                                         vkStencilOps);
        } else {
            cbInfo.fRenderPass = fGpu->resourceProvider().findRenderPass(*vkRT,
                                                                         vkColorOps,
                                                                         vkStencilOps);
        }

        SkASSERT(cbInfo.fRenderPass->isCompatible(*oldRP));
        oldRP->unref(fGpu);
        cbInfo.fBounds.join(fRenderTarget->getBoundsRect());
        cbInfo.fStartsWithClear = false;
    }
}

void GrVkGpuRTCommandBuffer::insertEventMarker(const char* msg) {
    // TODO: does Vulkan have a correlate?
}

void GrVkGpuRTCommandBuffer::onClearStencilClip(const GrFixedClip& clip, bool insideStencilMask) {
    SkASSERT(!clip.hasWindowRectangles());

    CommandBufferInfo& cbInfo = fCommandBufferInfos[fCurrentCmdInfo];

    GrStencilAttachment* sb = fRenderTarget->renderTargetPriv().getStencilAttachment();
    // this should only be called internally when we know we have a
    // stencil buffer.
    SkASSERT(sb);
    int stencilBitCount = sb->bits();

    // The contract with the callers does not guarantee that we preserve all bits in the stencil
    // during this clear. Thus we will clear the entire stencil to the desired value.

    VkClearDepthStencilValue vkStencilColor;
    memset(&vkStencilColor, 0, sizeof(VkClearDepthStencilValue));
    if (insideStencilMask) {
        vkStencilColor.stencil = (1 << (stencilBitCount - 1));
    } else {
        vkStencilColor.stencil = 0;
    }

    VkClearRect clearRect;
    // Flip rect if necessary
    SkIRect vkRect;
    if (!clip.scissorEnabled()) {
        vkRect.setXYWH(0, 0, fRenderTarget->width(), fRenderTarget->height());
    } else if (kBottomLeft_GrSurfaceOrigin != fOrigin) {
        vkRect = clip.scissorRect();
    } else {
        const SkIRect& scissor = clip.scissorRect();
        vkRect.setLTRB(scissor.fLeft, fRenderTarget->height() - scissor.fBottom,
                       scissor.fRight, fRenderTarget->height() - scissor.fTop);
    }

    clearRect.rect.offset = { vkRect.fLeft, vkRect.fTop };
    clearRect.rect.extent = { (uint32_t)vkRect.width(), (uint32_t)vkRect.height() };

    clearRect.baseArrayLayer = 0;
    clearRect.layerCount = 1;

    uint32_t stencilIndex;
    SkAssertResult(cbInfo.fRenderPass->stencilAttachmentIndex(&stencilIndex));

    VkClearAttachment attachment;
    attachment.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
    attachment.colorAttachment = 0; // this value shouldn't matter
    attachment.clearValue.depthStencil = vkStencilColor;

    cbInfo.currentCmdBuf()->clearAttachments(fGpu, 1, &attachment, 1, &clearRect);
    cbInfo.fIsEmpty = false;

    // Update command buffer bounds
    if (!clip.scissorEnabled()) {
        cbInfo.fBounds.join(fRenderTarget->getBoundsRect());
    } else {
        cbInfo.fBounds.join(SkRect::Make(clip.scissorRect()));
    }
}

void GrVkGpuRTCommandBuffer::onClear(const GrFixedClip& clip, GrColor color) {
    GrVkRenderTarget* vkRT = static_cast<GrVkRenderTarget*>(fRenderTarget);

    // parent class should never let us get here with no RT
    SkASSERT(!clip.hasWindowRectangles());

    CommandBufferInfo& cbInfo = fCommandBufferInfos[fCurrentCmdInfo];

    VkClearColorValue vkColor;
    GrColorToRGBAFloat(color, vkColor.float32);

    if (cbInfo.fIsEmpty && !clip.scissorEnabled()) {
        // Change the render pass to do a clear load
        GrVkRenderPass::LoadStoreOps vkColorOps(VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                VK_ATTACHMENT_STORE_OP_STORE);
        // Preserve the stencil buffer's load & store settings
        GrVkRenderPass::LoadStoreOps vkStencilOps(fVkStencilLoadOp, fVkStencilStoreOp);

        const GrVkRenderPass* oldRP = cbInfo.fRenderPass;

        const GrVkResourceProvider::CompatibleRPHandle& rpHandle =
            vkRT->compatibleRenderPassHandle();
        if (rpHandle.isValid()) {
            cbInfo.fRenderPass = fGpu->resourceProvider().findRenderPass(rpHandle,
                                                                         vkColorOps,
                                                                         vkStencilOps);
        } else {
            cbInfo.fRenderPass = fGpu->resourceProvider().findRenderPass(*vkRT,
                                                                         vkColorOps,
                                                                         vkStencilOps);
        }

        SkASSERT(cbInfo.fRenderPass->isCompatible(*oldRP));
        oldRP->unref(fGpu);

        GrColorToRGBAFloat(color, cbInfo.fColorClearValue.color.float32);
        cbInfo.fStartsWithClear = true;

        // Update command buffer bounds
        cbInfo.fBounds.join(fRenderTarget->getBoundsRect());
        return;
    }

    // We always do a sub rect clear with clearAttachments since we are inside a render pass
    VkClearRect clearRect;
    // Flip rect if necessary
    SkIRect vkRect;
    if (!clip.scissorEnabled()) {
        vkRect.setXYWH(0, 0, fRenderTarget->width(), fRenderTarget->height());
    } else if (kBottomLeft_GrSurfaceOrigin != fOrigin) {
        vkRect = clip.scissorRect();
    } else {
        const SkIRect& scissor = clip.scissorRect();
        vkRect.setLTRB(scissor.fLeft, fRenderTarget->height() - scissor.fBottom,
                       scissor.fRight, fRenderTarget->height() - scissor.fTop);
    }
    clearRect.rect.offset = { vkRect.fLeft, vkRect.fTop };
    clearRect.rect.extent = { (uint32_t)vkRect.width(), (uint32_t)vkRect.height() };
    clearRect.baseArrayLayer = 0;
    clearRect.layerCount = 1;

    uint32_t colorIndex;
    SkAssertResult(cbInfo.fRenderPass->colorAttachmentIndex(&colorIndex));

    VkClearAttachment attachment;
    attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    attachment.colorAttachment = colorIndex;
    attachment.clearValue.color = vkColor;

    cbInfo.currentCmdBuf()->clearAttachments(fGpu, 1, &attachment, 1, &clearRect);
    cbInfo.fIsEmpty = false;

    // Update command buffer bounds
    if (!clip.scissorEnabled()) {
        cbInfo.fBounds.join(fRenderTarget->getBoundsRect());
    } else {
        cbInfo.fBounds.join(SkRect::Make(clip.scissorRect()));
    }
    return;
}

////////////////////////////////////////////////////////////////////////////////

void GrVkGpuRTCommandBuffer::addAdditionalCommandBuffer() {
    GrVkRenderTarget* vkRT = static_cast<GrVkRenderTarget*>(fRenderTarget);

    CommandBufferInfo& cbInfo = fCommandBufferInfos[fCurrentCmdInfo];
    cbInfo.currentCmdBuf()->end(fGpu);
    cbInfo.fCommandBuffers.push_back(fGpu->resourceProvider().findOrCreateSecondaryCommandBuffer());
    cbInfo.currentCmdBuf()->begin(fGpu, vkRT->framebuffer(), cbInfo.fRenderPass);
}

void GrVkGpuRTCommandBuffer::addAdditionalRenderPass() {
    GrVkRenderTarget* vkRT = static_cast<GrVkRenderTarget*>(fRenderTarget);

    fCommandBufferInfos[fCurrentCmdInfo].currentCmdBuf()->end(fGpu);

    CommandBufferInfo& cbInfo = fCommandBufferInfos.push_back();
    fCurrentCmdInfo++;

    GrVkRenderPass::LoadStoreOps vkColorOps(VK_ATTACHMENT_LOAD_OP_LOAD,
                                            VK_ATTACHMENT_STORE_OP_STORE);
    GrVkRenderPass::LoadStoreOps vkStencilOps(VK_ATTACHMENT_LOAD_OP_LOAD,
                                              VK_ATTACHMENT_STORE_OP_STORE);

    const GrVkResourceProvider::CompatibleRPHandle& rpHandle =
            vkRT->compatibleRenderPassHandle();
    if (rpHandle.isValid()) {
        cbInfo.fRenderPass = fGpu->resourceProvider().findRenderPass(rpHandle,
                                                                     vkColorOps,
                                                                     vkStencilOps);
    } else {
        cbInfo.fRenderPass = fGpu->resourceProvider().findRenderPass(*vkRT,
                                                                     vkColorOps,
                                                                     vkStencilOps);
    }

    cbInfo.fCommandBuffers.push_back(fGpu->resourceProvider().findOrCreateSecondaryCommandBuffer());
    // It shouldn't matter what we set the clear color to here since we will assume loading of the
    // attachment.
    memset(&cbInfo.fColorClearValue, 0, sizeof(VkClearValue));
    cbInfo.fBounds.setEmpty();
    cbInfo.fIsEmpty = true;
    cbInfo.fStartsWithClear = false;

    cbInfo.currentCmdBuf()->begin(fGpu, vkRT->framebuffer(), cbInfo.fRenderPass);
}

void GrVkGpuRTCommandBuffer::inlineUpload(GrOpFlushState* state, GrDrawOp::DeferredUploadFn& upload) {

    if (!fCommandBufferInfos[fCurrentCmdInfo].fIsEmpty) {
        this->addAdditionalRenderPass();
    }
    fCommandBufferInfos[fCurrentCmdInfo].fPreDrawUploads.emplace_back(state, upload);
}

void GrVkGpuRTCommandBuffer::copy(GrSurface* src, const SkIRect& srcRect,
                                  const SkIPoint& dstPoint) {
    if (!fCommandBufferInfos[fCurrentCmdInfo].fIsEmpty ||
        fCommandBufferInfos[fCurrentCmdInfo].fStartsWithClear) {
        this->addAdditionalRenderPass();
    }
    fCommandBufferInfos[fCurrentCmdInfo].fPreCopies.emplace_back(src, srcRect, dstPoint);
}

////////////////////////////////////////////////////////////////////////////////

void GrVkGpuRTCommandBuffer::bindGeometry(const GrPrimitiveProcessor& primProc,
                                          const GrBuffer* indexBuffer,
                                          const GrBuffer* vertexBuffer,
                                          const GrBuffer* instanceBuffer) {
    GrVkSecondaryCommandBuffer* currCmdBuf = fCommandBufferInfos[fCurrentCmdInfo].currentCmdBuf();
    // There is no need to put any memory barriers to make sure host writes have finished here.
    // When a command buffer is submitted to a queue, there is an implicit memory barrier that
    // occurs for all host writes. Additionally, BufferMemoryBarriers are not allowed inside of
    // an active RenderPass.

    // Here our vertex and instance inputs need to match the same 0-based bindings they were
    // assigned in GrVkPipeline. That is, vertex first (if any) followed by instance.
    uint32_t binding = 0;

    if (primProc.hasVertexAttribs()) {
        SkASSERT(vertexBuffer);
        SkASSERT(!vertexBuffer->isCPUBacked());
        SkASSERT(!vertexBuffer->isMapped());

        currCmdBuf->bindInputBuffer(fGpu, binding++,
                                    static_cast<const GrVkVertexBuffer*>(vertexBuffer));
    }

    if (primProc.hasInstanceAttribs()) {
        SkASSERT(instanceBuffer);
        SkASSERT(!instanceBuffer->isCPUBacked());
        SkASSERT(!instanceBuffer->isMapped());

        currCmdBuf->bindInputBuffer(fGpu, binding++,
                                    static_cast<const GrVkVertexBuffer*>(instanceBuffer));
    }

    if (indexBuffer) {
        SkASSERT(indexBuffer);
        SkASSERT(!indexBuffer->isMapped());
        SkASSERT(!indexBuffer->isCPUBacked());

        currCmdBuf->bindIndexBuffer(fGpu, static_cast<const GrVkIndexBuffer*>(indexBuffer));
    }
}

sk_sp<GrVkPipelineState> GrVkGpuRTCommandBuffer::prepareDrawState(
                                                               const GrPipeline& pipeline,
                                                               const GrPrimitiveProcessor& primProc,
                                                               GrPrimitiveType primitiveType,
                                                               bool hasDynamicState) {
    CommandBufferInfo& cbInfo = fCommandBufferInfos[fCurrentCmdInfo];
    SkASSERT(cbInfo.fRenderPass);

    sk_sp<GrVkPipelineState> pipelineState =
        fGpu->resourceProvider().findOrCreateCompatiblePipelineState(pipeline,
                                                                     primProc,
                                                                     primitiveType,
                                                                     *cbInfo.fRenderPass);
    if (!pipelineState) {
        return pipelineState;
    }

    if (!cbInfo.fIsEmpty &&
        fLastPipelineState && fLastPipelineState != pipelineState.get() &&
        fGpu->vkCaps().newCBOnPipelineChange()) {
        this->addAdditionalCommandBuffer();
    }
    fLastPipelineState = pipelineState.get();

    pipelineState->setData(fGpu, primProc, pipeline);

    pipelineState->bind(fGpu, cbInfo.currentCmdBuf());

    GrRenderTarget* rt = pipeline.renderTarget();

    if (!pipeline.getScissorState().enabled()) {
        GrVkPipeline::SetDynamicScissorRectState(fGpu, cbInfo.currentCmdBuf(), rt,
                                                 SkIRect::MakeWH(rt->width(), rt->height()));
    } else if (!hasDynamicState) {
        GrVkPipeline::SetDynamicScissorRectState(fGpu, cbInfo.currentCmdBuf(), rt,
                                                 pipeline.getScissorState().rect());
    }
    GrVkPipeline::SetDynamicViewportState(fGpu, cbInfo.currentCmdBuf(), rt);
    GrVkPipeline::SetDynamicBlendConstantState(fGpu, cbInfo.currentCmdBuf(), rt->config(),
                                               pipeline.getXferProcessor());

    return pipelineState;
}

static void set_texture_layout(GrVkTexture* vkTexture, GrVkGpu* gpu) {
    // TODO: If we ever decide to create the secondary command buffers ahead of time before we
    // are actually going to submit them, we will need to track the sampled images and delay
    // adding the layout change/barrier until we are ready to submit.
    vkTexture->setImageLayout(gpu,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_ACCESS_SHADER_READ_BIT,
                              VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                              false);
}

static void prepare_sampled_images(const GrResourceIOProcessor& processor, GrVkGpu* gpu) {
    for (int i = 0; i < processor.numTextureSamplers(); ++i) {
        const GrResourceIOProcessor::TextureSampler& sampler = processor.textureSampler(i);
        GrVkTexture* vkTexture = static_cast<GrVkTexture*>(sampler.peekTexture());

        // We may need to resolve the texture first if it is also a render target
        GrVkRenderTarget* texRT = static_cast<GrVkRenderTarget*>(vkTexture->asRenderTarget());
        if (texRT) {
            gpu->onResolveRenderTarget(texRT);
        }

        const GrSamplerParams& params = sampler.params();
        // Check if we need to regenerate any mip maps
        if (GrSamplerParams::kMipMap_FilterMode == params.filterMode()) {
            if (vkTexture->texturePriv().mipMapsAreDirty()) {
                gpu->generateMipmap(vkTexture);
                vkTexture->texturePriv().dirtyMipMaps(false);
            }
        }
        set_texture_layout(vkTexture, gpu);
    }
}

void GrVkGpuRTCommandBuffer::onDraw(const GrPipeline& pipeline,
                                    const GrPrimitiveProcessor& primProc,
                                    const GrMesh meshes[],
                                    const GrPipeline::DynamicState dynamicStates[],
                                    int meshCount,
                                    const SkRect& bounds) {
    SkASSERT(pipeline.renderTarget() == fRenderTarget);

    if (!meshCount) {
        return;
    }
    prepare_sampled_images(primProc, fGpu);
    GrFragmentProcessor::Iter iter(pipeline);
    while (const GrFragmentProcessor* fp = iter.next()) {
        prepare_sampled_images(*fp, fGpu);
    }
    if (GrTexture* dstTexture = pipeline.peekDstTexture()) {
        set_texture_layout(static_cast<GrVkTexture*>(dstTexture), fGpu);
    }

    GrPrimitiveType primitiveType = meshes[0].primitiveType();
    sk_sp<GrVkPipelineState> pipelineState = this->prepareDrawState(pipeline,
                                                                    primProc,
                                                                    primitiveType,
                                                                    SkToBool(dynamicStates));
    if (!pipelineState) {
        return;
    }

    CommandBufferInfo& cbInfo = fCommandBufferInfos[fCurrentCmdInfo];

    for (int i = 0; i < meshCount; ++i) {
        const GrMesh& mesh = meshes[i];
        if (mesh.primitiveType() != primitiveType) {
            // Technically we don't have to call this here (since there is a safety check in
            // pipelineState:setData but this will allow for quicker freeing of resources if the
            // pipelineState sits in a cache for a while.
            pipelineState->freeTempResources(fGpu);
            SkDEBUGCODE(pipelineState = nullptr);
            primitiveType = mesh.primitiveType();
            pipelineState = this->prepareDrawState(pipeline,
                                                   primProc,
                                                   primitiveType,
                                                   SkToBool(dynamicStates));
            if (!pipelineState) {
                return;
            }
        }

        if (dynamicStates) {
            if (pipeline.getScissorState().enabled()) {
                GrVkPipeline::SetDynamicScissorRectState(fGpu, cbInfo.currentCmdBuf(),
                                                         fRenderTarget,
                                                         dynamicStates[i].fScissorRect);
            }
        }

        SkASSERT(pipelineState);
        mesh.sendToGpu(primProc, this);
    }

    cbInfo.fBounds.join(bounds);
    cbInfo.fIsEmpty = false;

    // Technically we don't have to call this here (since there is a safety check in
    // pipelineState:setData but this will allow for quicker freeing of resources if the
    // pipelineState sits in a cache for a while.
    pipelineState->freeTempResources(fGpu);
}

void GrVkGpuRTCommandBuffer::sendInstancedMeshToGpu(const GrPrimitiveProcessor& primProc,
                                                    GrPrimitiveType,
                                                    const GrBuffer* vertexBuffer,
                                                    int vertexCount,
                                                    int baseVertex,
                                                    const GrBuffer* instanceBuffer,
                                                    int instanceCount,
                                                    int baseInstance) {
    CommandBufferInfo& cbInfo = fCommandBufferInfos[fCurrentCmdInfo];
    this->bindGeometry(primProc, nullptr, vertexBuffer, instanceBuffer);
    cbInfo.currentCmdBuf()->draw(fGpu, vertexCount, instanceCount, baseVertex, baseInstance);
    fGpu->stats()->incNumDraws();
}

void GrVkGpuRTCommandBuffer::sendIndexedInstancedMeshToGpu(const GrPrimitiveProcessor& primProc,
                                                           GrPrimitiveType,
                                                           const GrBuffer* indexBuffer,
                                                           int indexCount,
                                                           int baseIndex,
                                                           const GrBuffer* vertexBuffer,
                                                           int baseVertex,
                                                           const GrBuffer* instanceBuffer,
                                                           int instanceCount,
                                                           int baseInstance) {
    CommandBufferInfo& cbInfo = fCommandBufferInfos[fCurrentCmdInfo];
    this->bindGeometry(primProc, indexBuffer, vertexBuffer, instanceBuffer);
    cbInfo.currentCmdBuf()->drawIndexed(fGpu, indexCount, instanceCount,
                                        baseIndex, baseVertex, baseInstance);
    fGpu->stats()->incNumDraws();
}

