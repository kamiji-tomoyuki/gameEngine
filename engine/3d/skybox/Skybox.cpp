#include "Skybox.h"
#include "TextureManager.h"
#include "myMath.h"
#include <cassert>

void Skybox::Initialize(const std::string &textureFilePath) {
    // 基盤オブジェクトの取得
    dxCommon_ = DirectXCommon::GetInstance();
    srvManager_ = SrvManager::GetInstance();

    // テクスチャファイルパスの設定
    textureFilePath_ = "resources/images/" + textureFilePath;

    // 頂点データとインデックスデータの作成
    CreateVertexData();
    CreateIndexData();
    CreateMaterial();
    CreateTransformationMatrix();

    // パイプラインマネージャーの初期化
    psoManager_ = std::make_unique<PipeLineManager>();
    psoManager_->Initialize(dxCommon_);

    // ルートシグネチャの作成
    rootSignature = psoManager_->CreateSkyboxRootSignature(rootSignature);

    // グラフィックスパイプラインの作成（各ブレンドモード用）
    graphicsPipelineState[0] = psoManager_->CreateSkyboxGraphicsPipeLine(graphicsPipelineState[0], rootSignature, BlendMode::kNormal);
    graphicsPipelineState[1] = psoManager_->CreateSkyboxGraphicsPipeLine(graphicsPipelineState[1], rootSignature, BlendMode::kAdd);
    graphicsPipelineState[2] = psoManager_->CreateSkyboxGraphicsPipeLine(graphicsPipelineState[2], rootSignature, BlendMode::kSubtract);
    graphicsPipelineState[3] = psoManager_->CreateSkyboxGraphicsPipeLine(graphicsPipelineState[3], rootSignature, BlendMode::kMultiply);
    graphicsPipelineState[4] = psoManager_->CreateSkyboxGraphicsPipeLine(graphicsPipelineState[4], rootSignature, BlendMode::kScreen);

    // DDSテクスチャの読み込み
    TextureManager::GetInstance()->LoadTexture(textureFilePath_);
    textureIndex_ = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath_);
}

void Skybox::Update(const ViewProjection &viewProjection) {
    // スカイボックスは常にカメラを中心に配置
    // 平行移動成分を除去したビュー行列を作成
    Matrix4x4 skyboxViewMatrix = viewProjection.matView_;
    skyboxViewMatrix.m[3][0] = 0.0f; // X軸の平行移動を除去
    skyboxViewMatrix.m[3][1] = 0.0f; // Y軸の平行移動を除去
    skyboxViewMatrix.m[3][2] = 0.0f; // Z軸の平行移動を除去

    // スケール行列を作成
    Matrix4x4 scaleMatrix = MakeScaleMatrix({scale_, scale_, scale_});

    // 変換行列の計算
    Matrix4x4 worldMatrix = scaleMatrix;
    Matrix4x4 worldViewProjectionMatrix = worldMatrix * skyboxViewMatrix * viewProjection.matProjection_;

    // 変換行列データの更新
    transformationMatrixData->WVP = worldViewProjectionMatrix;
    transformationMatrixData->World = worldMatrix;
    transformationMatrixData->WorldInverseTranspose = Transpose(Inverse(worldMatrix));

    // マテリアルデータの更新
    materialData->color = {1.0f, 1.0f, 1.0f, 1.0f};
    materialData->uvTransform = MakeIdentity4x4();
}

void Skybox::Draw() {
    // 描画前の状態保存
    auto commandList = dxCommon_->GetCommandList();

    // 深度テストの設定（深度値を1.0で書き込み、深度テストはLessEqual）
    // スカイボックスは背景なので、深度バッファをクリア後に1.0で書き込む

    // ブレンドモード設定
    SetBlendMode(blendMode_);

    // 定数バッファの設定
    commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());

    // 頂点バッファの設定
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->IASetIndexBuffer(&indexBufferView);

    // テクスチャの設定
    srvManager_->SetGraphicsRootDescriptorTable(2, textureIndex_);

    // プリミティブトポロジーの設定
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 描画
    commandList->DrawIndexedInstanced(static_cast<UINT>(indices_.size()), 1, 0, 0, 0);
}

void Skybox::SetBlendMode(BlendMode blendMode) {
    blendMode_ = blendMode;

    switch (blendMode) {
    case BlendMode::kNormal:
        psoManager_->DrawCommonSetting(graphicsPipelineState[0], rootSignature);
        break;
    case BlendMode::kAdd:
        psoManager_->DrawCommonSetting(graphicsPipelineState[1], rootSignature);
        break;
    case BlendMode::kSubtract:
        psoManager_->DrawCommonSetting(graphicsPipelineState[2], rootSignature);
        break;
    case BlendMode::kMultiply:
        psoManager_->DrawCommonSetting(graphicsPipelineState[3], rootSignature);
        break;
    case BlendMode::kScreen:
        psoManager_->DrawCommonSetting(graphicsPipelineState[4], rootSignature);
        break;
    default:
        psoManager_->DrawCommonSetting(graphicsPipelineState[0], rootSignature);
        break;
    }
}

void Skybox::CreateVertexData() {
    // 頂点データの作成（立方体の内側向き）
    std::vector<VertexData> vertices;
    vertices.resize(24); // 6面 * 4頂点

    float hs = 1.0f; // ハーフサイズ

    // 右面 (+X) - 内側向きなので頂点順序を逆に
    vertices[0] = {{hs, hs, hs, 1.0f}, uvs_[0]};
    vertices[1] = {{hs, -hs, hs, 1.0f}, uvs_[3]};
    vertices[2] = {{hs, -hs, -hs, 1.0f}, uvs_[2]};
    vertices[3] = {{hs, hs, -hs, 1.0f}, uvs_[1]};

    // 左面 (-X) - 内側向きなので頂点順序を逆に
    vertices[4] = {{-hs, hs, -hs, 1.0f}, uvs_[0]};
    vertices[5] = {{-hs, -hs, -hs, 1.0f}, uvs_[3]};
    vertices[6] = {{-hs, -hs, hs, 1.0f}, uvs_[2]};
    vertices[7] = {{-hs, hs, hs, 1.0f}, uvs_[1]};

    // 前面 (+Z) - 内側向きなので頂点順序を逆に
    vertices[8] = {{-hs, hs, hs, 1.0f}, uvs_[0]};
    vertices[9] = {{-hs, -hs, hs, 1.0f}, uvs_[3]};
    vertices[10] = {{hs, -hs, hs, 1.0f}, uvs_[2]};
    vertices[11] = {{hs, hs, hs, 1.0f}, uvs_[1]};

    // 背面 (-Z) - 内側向きなので頂点順序を逆に
    vertices[12] = {{hs, hs, -hs, 1.0f}, uvs_[0]};
    vertices[13] = {{hs, -hs, -hs, 1.0f}, uvs_[3]};
    vertices[14] = {{-hs, -hs, -hs, 1.0f}, uvs_[2]};
    vertices[15] = {{-hs, hs, -hs, 1.0f}, uvs_[1]};

    // 上面 (+Y) - 内側向きなので頂点順序を逆に
    vertices[16] = {{-hs, hs, -hs, 1.0f}, uvs_[0]};
    vertices[17] = {{-hs, hs, hs, 1.0f}, uvs_[3]};
    vertices[18] = {{hs, hs, hs, 1.0f}, uvs_[2]};
    vertices[19] = {{hs, hs, -hs, 1.0f}, uvs_[1]};

    // 下面 (-Y) - 内側向きなので頂点順序を逆に
    vertices[20] = {{-hs, -hs, hs, 1.0f}, uvs_[0]};
    vertices[21] = {{-hs, -hs, -hs, 1.0f}, uvs_[3]};
    vertices[22] = {{hs, -hs, -hs, 1.0f}, uvs_[2]};
    vertices[23] = {{hs, -hs, hs, 1.0f}, uvs_[1]};

    // 頂点バッファの作成
    vertexResource = dxCommon_->CreateBufferResource(sizeof(VertexData) * vertices.size());
    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * vertices.size());
    vertexBufferView.StrideInBytes = sizeof(VertexData);

    // 頂点データのマッピング
    vertexResource->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));
    std::memcpy(vertexData, vertices.data(), sizeof(VertexData) * vertices.size());
}

void Skybox::CreateIndexData() {
    // インデックスデータの作成（内側向きの面なので時計回り）
    indices_ = {
        // 右面
        0, 1, 2, 0, 2, 3,
        // 左面
        4, 5, 6, 4, 6, 7,
        // 前面
        8, 9, 10, 8, 10, 11,
        // 背面
        12, 13, 14, 12, 14, 15,
        // 上面
        16, 17, 18, 16, 18, 19,
        // 下面
        20, 21, 22, 20, 22, 23};

    // インデックスバッファの作成
    indexResource = dxCommon_->CreateBufferResource(sizeof(uint32_t) * indices_.size());
    indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
    indexBufferView.SizeInBytes = UINT(sizeof(uint32_t) * indices_.size());
    indexBufferView.Format = DXGI_FORMAT_R32_UINT;

    // インデックスデータのマッピング
    indexResource->Map(0, nullptr, reinterpret_cast<void **>(&indexData));
    std::memcpy(indexData, indices_.data(), sizeof(uint32_t) * indices_.size());
}

void Skybox::CreateMaterial() {
    // マテリアルリソースの作成
    materialResource = dxCommon_->CreateBufferResource(sizeof(Material));
    materialResource->Map(0, nullptr, reinterpret_cast<void **>(&materialData));

    // マテリアルデータの初期化
    materialData->color = {1.0f, 1.0f, 1.0f, 1.0f};
    materialData->uvTransform = MakeIdentity4x4();
}

void Skybox::CreateTransformationMatrix() {
    // 変換行列リソースの作成
    transformationMatrixResource = dxCommon_->CreateBufferResource(sizeof(TransformationMatrix));
    transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void **>(&transformationMatrixData));

    // 変換行列データの初期化
    transformationMatrixData->WVP = MakeIdentity4x4();
    transformationMatrixData->World = MakeIdentity4x4();
    transformationMatrixData->WorldInverseTranspose = MakeIdentity4x4();
}