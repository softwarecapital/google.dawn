// Copyright 2019 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "tests/DawnTest.h"

#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/WGPUHelpers.h"

constexpr uint32_t kRTSize = 4;

class DestroyTest : public DawnTest {
  protected:
    void SetUp() override {
        DawnTest::SetUp();
        DAWN_SKIP_TEST_IF(HasToggleEnabled("skip_validation"));

        renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

        wgpu::ShaderModule vsModule = utils::CreateShaderModule(device, R"(
              [[stage(vertex)]]
              fn main([[location(0)]] pos : vec4<f32>) -> [[builtin(position)]] vec4<f32> {
                  return pos;
              })");

        wgpu::ShaderModule fsModule = utils::CreateShaderModule(device, R"(
              [[stage(fragment)]] fn main() -> [[location(0)]] vec4<f32> {
                  return vec4<f32>(0.0, 1.0, 0.0, 1.0);
              })");

        utils::ComboRenderPipelineDescriptor2 descriptor;
        descriptor.vertex.module = vsModule;
        descriptor.cFragment.module = fsModule;
        descriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        descriptor.vertex.bufferCount = 1;
        descriptor.cBuffers[0].arrayStride = 4 * sizeof(float);
        descriptor.cBuffers[0].attributeCount = 1;
        descriptor.cAttributes[0].format = wgpu::VertexFormat::Float32x4;
        descriptor.cTargets[0].format = renderPass.colorFormat;

        pipeline = device.CreateRenderPipeline2(&descriptor);

        vertexBuffer = utils::CreateBufferFromData<float>(
            device, wgpu::BufferUsage::Vertex,
            {// The bottom left triangle
             -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 1.0f});

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.BeginRenderPass(&renderPass.renderPassInfo).EndPass();
        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);
    }

    utils::BasicRenderPass renderPass;
    wgpu::RenderPipeline pipeline;
    wgpu::Buffer vertexBuffer;

    wgpu::CommandBuffer CreateTriangleCommandBuffer() {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
            pass.SetPipeline(pipeline);
            pass.SetVertexBuffer(0, vertexBuffer);
            pass.Draw(3);
            pass.EndPass();
        }
        wgpu::CommandBuffer commands = encoder.Finish();
        return commands;
    }
};

// Destroy before submit will result in error, and nothing drawn
TEST_P(DestroyTest, BufferDestroyBeforeSubmit) {
    RGBA8 notFilled(0, 0, 0, 0);

    wgpu::CommandBuffer commands = CreateTriangleCommandBuffer();
    vertexBuffer.Destroy();
    ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));

    EXPECT_PIXEL_RGBA8_EQ(notFilled, renderPass.color, 1, 3);
}

// Destroy after submit will draw successfully
TEST_P(DestroyTest, BufferDestroyAfterSubmit) {
    RGBA8 filled(0, 255, 0, 255);

    wgpu::CommandBuffer commands = CreateTriangleCommandBuffer();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, 1, 3);
    vertexBuffer.Destroy();
}

// First submit succeeds, draws triangle, second submit fails
// after destroy is called on the buffer, pixel does not change
TEST_P(DestroyTest, BufferSubmitDestroySubmit) {
    RGBA8 filled(0, 255, 0, 255);

    wgpu::CommandBuffer commands = CreateTriangleCommandBuffer();
    queue.Submit(1, &commands);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, 1, 3);

    vertexBuffer.Destroy();

    // Submit fails because vertex buffer was destroyed
    ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));

    // Pixel stays the same
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, 1, 3);
}

// Destroy texture before submit should fail submit
TEST_P(DestroyTest, TextureDestroyBeforeSubmit) {
    wgpu::CommandBuffer commands = CreateTriangleCommandBuffer();
    renderPass.color.Destroy();
    ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));
}

// Destroy after submit will draw successfully
TEST_P(DestroyTest, TextureDestroyAfterSubmit) {
    RGBA8 filled(0, 255, 0, 255);

    wgpu::CommandBuffer commands = CreateTriangleCommandBuffer();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, 1, 3);
    renderPass.color.Destroy();
}

// First submit succeeds, draws triangle, second submit fails
// after destroy is called on the texture
TEST_P(DestroyTest, TextureSubmitDestroySubmit) {
    RGBA8 filled(0, 255, 0, 255);

    wgpu::CommandBuffer commands = CreateTriangleCommandBuffer();
    queue.Submit(1, &commands);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, 1, 3);

    renderPass.color.Destroy();

    // Submit fails because texture was destroyed
    ASSERT_DEVICE_ERROR(queue.Submit(1, &commands));
}

DAWN_INSTANTIATE_TEST(DestroyTest,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      OpenGLESBackend(),
                      VulkanBackend());
