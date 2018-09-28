#include "qeheader.h"



void QeModel::initialize(QeAssetXML* _property, QeObject* _owner) {
	QeComponent::initialize(_property, _owner);

	AST->getXMLbValue(&graphicsPipeline.bAlpha, initProperty, 1, "alpha");
	AST->getXMLfValue(&bufferData.param.z, initProperty, 1, "lineWidth");

	VK->createBuffer(modelBuffer, sizeof(bufferData), nullptr);

	const char* c = AST->getXMLValue(initProperty, 1, "obj");
	modelData = AST->getModel(c);
	materialData = modelData->pMaterial;
	if(materialData) bufferData.material = materialData->value;

	AST->setGraphicsShader(graphicsShader, nullptr, "model");
	AST->setGraphicsShader(normalShader, nullptr, "normal");
	AST->setGraphicsShader(outlineShader, nullptr, "outline");

	AST->getXMLiValue(&materialOID, initProperty, 1, "materialOID");

	bUpdateMaterialOID = false;
	graphicsPipeline.bAlpha = false;
	GRAP->models.push_back(this);
}

void QeModel::clear() {
	if (graphicsPipeline.bAlpha)	eraseElementFromVector<QeModel*>(GRAP->alphaModels, this);
	else							eraseElementFromVector<QeModel*>(GRAP->models, this);
}

void QeModel::update1() {

	if (materialOID && !bUpdateMaterialOID) {

		QeMaterial * material = (QeMaterial*)OBJMGR->findComponent( eComponent_material, materialOID);
		if (material) {
			bUpdateMaterialOID = true;

			if (graphicsPipeline.bAlpha != material->bAlpha) {
				graphicsPipeline.bAlpha = material->bAlpha;
				if (graphicsPipeline.bAlpha) {
					eraseElementFromVector<QeModel*>(GRAP->models, this);
					GRAP->alphaModels.push_back(this);
				}
				else {
					eraseElementFromVector<QeModel*>(GRAP->alphaModels, this);
					GRAP->models.push_back(this);
				}
			}
			materialData = &material->materialData;
			AST->setGraphicsShader(graphicsShader, material->initProperty, "model");
			bufferData.material = materialData->value;
			VK->updateDescriptorSet(&createDescriptorSetModel(), descriptorSet);
		}
	}

	bufferData.model = owner->transform->worldTransformMatrix(bRotate);

	VK->setMemoryBuffer(modelBuffer, sizeof(bufferData), &bufferData);
}
void QeModel::update2() {}


void QeModel::updateShaderData() {

	if (!descriptorSet.set) {
		VK->createDescriptorSet(descriptorSet);
		VK->updateDescriptorSet(&createDescriptorSetModel(), descriptorSet);
	}
}

QeDataDescriptorSetModel QeModel::createDescriptorSetModel() {
	QeDataDescriptorSetModel descriptorSetData;
	descriptorSetData.modelBuffer = modelBuffer.buffer;

	bufferData.param = { 0,0,0,0 };
	if (materialData) {
		if (materialData->image.pBaseColorMap) {
			descriptorSetData.baseColorMapImageView = materialData->image.pBaseColorMap->view;
			descriptorSetData.baseColorMapSampler = materialData->image.pBaseColorMap->sampler;
			bufferData.param.x = 1;
		}
		if (materialData->image.pNormalMap) {
			descriptorSetData.normalMapImageView = materialData->image.pNormalMap->view;
			descriptorSetData.normalMapSampler = materialData->image.pNormalMap->sampler;
			bufferData.param.y = 1;
		}
		if (materialData->image.pCubeMap) {
			descriptorSetData.cubeMapImageView = materialData->image.pCubeMap->view;
			descriptorSetData.cubeMapSampler = materialData->image.pCubeMap->sampler;
			bufferData.param.z = 1;
		}
	}
	return descriptorSetData;
}

void QeModel::createPipeline() {
	if (computeShader) computePipeline = VK->createComputePipeline(computeShader);
}

bool QeModel::isShowByCulling(QeCamera* camera) {

	bool _bCullingShow = true;
	QeVector3f vec = owner->transform->worldPosition() - camera->owner->transform->worldPosition();
	float dis = MATH->length(vec);

	if (dis > camera->cullingDistance) return false;

	if (componentType != eComponent_cubemap && _bCullingShow) {
		float angle = MATH->getAnglefromVectors(camera->face(), vec);
		if (angle > 100 || angle < -100) return false;
	}

	return true;
}


void QeModel::updateDrawCommandBuffer(QeDataDrawCommand* command) {

	if (!isShowByCulling(command->camera)) return;

	vkCmdBindDescriptorSets(command->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VK->pipelineLayout, 0, 1, &descriptorSet.set, 0, nullptr);

	VkDeviceSize offsets[] = { 0 };

	graphicsPipeline.subpass = 0;
	graphicsPipeline.componentType = componentType;
	graphicsPipeline.sampleCount = GRAP->sampleCount;
	graphicsPipeline.renderPass = command->renderPass;
	graphicsPipeline.minorType = eGraphicsPipeLine_none;
	graphicsPipeline.shader = &graphicsShader;

	vkCmdBindPipeline(command->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VK->createGraphicsPipeline(&graphicsPipeline));

	switch (componentType) {
	case eComponent_model:
	case eComponent_cubemap:
	case eComponent_animation:
		vkCmdBindVertexBuffers(command->commandBuffer, 0, 1, &modelData->vertex.buffer, offsets);
		vkCmdBindIndexBuffer(command->commandBuffer, modelData->index.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(command->commandBuffer, static_cast<uint32_t>(modelData->indexSize), 1, 0, 0, 0);
		break;

	case eComponent_render:
	case eComponent_billboard:
		vkCmdDraw(command->commandBuffer, 1, 1, 0, 0);
		break;

	case eComponent_line:
	case eComponent_axis:
	case eComponent_grid:
		vkCmdBindVertexBuffers(command->commandBuffer, 0, 1, &modelData->vertex.buffer, offsets);
		vkCmdDraw(command->commandBuffer, uint32_t(modelData->vertices.size()), 1, 0, 0);
		break;

		//case eObject_Particle:
		//	break;
	}

	if (bufferData.param.z  && outlineShader.vert) {

		graphicsPipeline.minorType = eGraphicsPipeLine_stencilBuffer;
		graphicsPipeline.shader = &outlineShader;

		vkCmdBindPipeline(command->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VK->createGraphicsPipeline(&graphicsPipeline));
		vkCmdDrawIndexed(command->commandBuffer, static_cast<uint32_t>(modelData->indexSize), 1, 0, 0, 0);
	}

	if (VK->bShowNormal && normalShader.vert) {

		graphicsPipeline.minorType = eGraphicsPipeLine_normal;
		graphicsPipeline.shader = &normalShader;

		vkCmdBindPipeline(command->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VK->createGraphicsPipeline(&graphicsPipeline));
		vkCmdDraw(command->commandBuffer, uint32_t(modelData->vertices.size()), 1, 0, 0);
	}
}