#pragma once

#include "qeheader.h"

struct QeDataEnvironment {
    QeDataCamera camera;
    QeVector4f param;  // 0: gamma, 1: exposure
};

struct QeDataViewport {
    VkViewport viewport;
    VkRect2D scissor;
    QeCamera *camera = nullptr;
    // std::vector<QeLight*> lights;

    QeDataDescriptorSet descriptorSetComputeRayTracing;
    QeDataComputePipeline computePipelineRayTracing;

    QeDataDescriptorSet commonDescriptorSet;
    QeDataEnvironment environmentData;
    QeVKBuffer environmentBuffer;

    QeDataViewport()
        : environmentBuffer(eBuffer_uniform),
          commonDescriptorSet(eDescriptorSetLayout_Common),
          descriptorSetComputeRayTracing(eDescriptorSetLayout_Raytracing) {}

    //~QeDataViewport();
};

struct QeBufferSubpass {
    QeVector4f param1;  // 0: blurHorizontal, 1: blurScale, 2: blurStrength
};

struct QeDataSubpass {
    QeBufferSubpass bufferData;
    QeVKBuffer buffer;
    QeAssetGraphicsShader graphicsShader;
    QeDataDescriptorSet descriptorSet;
    QeDataGraphicsPipeline graphicsPipeline;

    QeDataSubpass() : buffer(eBuffer_uniform), descriptorSet(eDescriptorSetLayout_Postprocessing) {}
    //~QeDataSubpass();
};

struct QeDataRender {
    std::vector<QeDataViewport *> viewports;
    int cameraOID;
    VkViewport viewport;
    VkRect2D scissor;

    QeVKImage colorImage, colorImage2, depthStencilImage,
        multiSampleColorImage;  // , multiSampleDepthStencilImage;
    std::vector<VkFramebuffer> frameBuffers;

    std::vector<VkCommandBuffer> commandBuffers;
    VkSemaphore semaphore = VK_NULL_HANDLE;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<QeDataSubpass *> subpass;

    QeDataRender()
        : colorImage(eImage_render),
          colorImage2(eImage_render),
          depthStencilImage(eImage_depthStencil),
          multiSampleColorImage(eImage_attach) /*,
                                                  multiSampleDepthStencilImage(eImage_multiSampleDepthStencil)*/
    {}
    ~QeDataRender();
    void clear();
};

struct QeDataSwapchain {
    VkSwapchainKHR khr = VK_NULL_HANDLE;
    VkExtent2D extent;
    VkFormat format;
    std::vector<QeVKImage> images;
};

struct QeDataDrawCommand {
    VkCommandBuffer commandBuffer;
    QeCamera *camera;
    QeDataDescriptorSet *commonDescriptorSet;
    VkRenderPass renderPass;
    QeRenderType type;
};

class QeGraphics {
   public:
    QeGraphics(QeGlobalKey &_key) : lightsBuffer(eBuffer_storage), modelDatasBuffer(eBuffer_storage) {}
    ~QeGraphics();

    std::vector<QeDataRender *> renders;
    QeVector3f clearColor;
    QeDataSwapchain swapchain;

    // int currentTargetViewport = 0;
    QeCamera *currentCamera = nullptr;
    VkSemaphore renderCompleteSemaphore = VK_NULL_HANDLE;
    std::vector<VkFence> fences;
    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;

    std::vector<QeModel *> models;
    std::vector<QeModel *> alphaModels;
    std::vector<QeModel *> models2D;
    std::vector<QeLight *> lights;
    QeVKBuffer modelDatasBuffer;
    QeVKBuffer lightsBuffer;
    bool bUpdateLight = false;

    void initialize();
    void clear();
    bool addPostProcssing(QeRenderType renderType, int cameraOID, int postprocessingOID);
    void addNewViewport(QeRenderType type);
    void popViewport(QeRenderType type);
    void updateViewport();
    void updateBuffer();
    void add2DModel(QeModel *model);
    void addLight(QeLight *light);
    void removeLight(QeLight *light);
    void update1();
    void update2();
    void setTargetCamera(int cameraOID);
    QeCamera *getTargetCamera();
    QeDataRender *getRender(QeRenderType type, int cameraOID);
    // bool bUpdateComputeCommandBuffers = false;
    // bool bUpdateDrawCommandBuffers = false;
    bool bRecreateRender = false;

    // std::vector<VkSemaphore> computeSemaphores;
    // std::vector<VkFence> computeFences;

    // VkSemaphore textOverlayComplete;

    QeDataRender *createRender(QeRenderType type, int cameraOID, VkExtent2D renderSize);
    void refreshRender();
    void cleanupRender();
    void drawFrame();
    void updateDrawCommandBuffers();
    // void updateComputeCommandBuffers();

    void sortAlphaModels(QeCamera *camera);

    void updateComputeCommandBuffer(VkCommandBuffer &commandBuffer);
    void updateDrawCommandBuffer(QeDataDrawCommand *command);
};