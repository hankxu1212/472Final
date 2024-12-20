#include "backend/VulkanContext.hpp"

CommandBuffer::CommandBuffer(bool begin, VkQueueFlagBits queueType, VkCommandBufferLevel bufferLevel) :
	r_CommandPool(VulkanContext::Get()->GetCommandPool()),
	m_QueueType(queueType)
{
	VkCommandBufferAllocateInfo cmdBufferInfo = {};
	cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferInfo.commandPool = *r_CommandPool;
	cmdBufferInfo.level = bufferLevel;
	cmdBufferInfo.commandBufferCount = 1;
	VulkanContext::VK(vkAllocateCommandBuffers(VulkanContext::GetDevice(), &cmdBufferInfo, &m_CommandBuffer),
		"[vulkan] Error: cannot create command buffers");

	if (begin)
		Begin();
}

CommandBuffer::~CommandBuffer() 
{
	vkFreeCommandBuffers(VulkanContext::GetDevice(), *r_CommandPool, 1, &m_CommandBuffer);
}

void CommandBuffer::Begin(VkCommandBufferUsageFlags usage, const VkCommandBufferInheritanceInfo* inheritance)
{
	if (running)
		return;

	VkCommandBufferBeginInfo beginInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = usage,
		.pInheritanceInfo = inheritance
	};
	VulkanContext::VK(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo), 
		"[vulkan] Error: cannot begin command buffer");

	running = true;
}

void CommandBuffer::End() {
	if (!running) return;

	VulkanContext::VK(vkEndCommandBuffer(m_CommandBuffer), 
		"[vulkan] Error: cannot end command buffer");
	running = false;
}

void CommandBuffer::SubmitIdle() {
	if (running)
		End();

	VkSubmitInfo submitInfo {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &m_CommandBuffer
	};

	vkQueueSubmit(GetQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(GetQueue());
}

void CommandBuffer::SubmitWait()
{
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	VkFence fence;
	VulkanContext::VK(vkCreateFence(VulkanContext::GetDevice(), &fenceCreateInfo, nullptr, &fence));
	Submit(VK_NULL_HANDLE, VK_NULL_HANDLE, fence);

	vkQueueWaitIdle(GetQueue());
	vkWaitForFences(VulkanContext::GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
	vkDestroyFence(VulkanContext::GetDevice(), fence, nullptr);
}

void CommandBuffer::Submit(const VkSemaphore& waitSemaphore, const VkSemaphore& signalSemaphore, VkFence fence) {
	if (running)
		End();

	VkSubmitInfo submitInfo {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &m_CommandBuffer
	};

	if (waitSemaphore != VK_NULL_HANDLE) {
		// Pipeline stages used to Wait at for graphics queue submissions.
		static VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		submitInfo.pWaitDstStageMask = &submitPipelineStages;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &waitSemaphore;
	}

	if (signalSemaphore != VK_NULL_HANDLE) {
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &signalSemaphore;
	}

	if (fence != VK_NULL_HANDLE)
		vkResetFences(VulkanContext::GetDevice(), 1, &fence);

	vkQueueSubmit(GetQueue(), 1, &submitInfo, fence);
}

VkQueue CommandBuffer::GetQueue() const 
{
	switch (m_QueueType) 
	{
	case VK_QUEUE_GRAPHICS_BIT:
		return VulkanContext::Get()->getLogicalDevice()->getGraphicsQueue();
	case VK_QUEUE_COMPUTE_BIT:
		return VulkanContext::Get()->getLogicalDevice()->getComputeQueue();
	case VK_QUEUE_TRANSFER_BIT:
		return VulkanContext::Get()->getLogicalDevice()->getTransferQueue();
	default:
		return nullptr;
	}
}
