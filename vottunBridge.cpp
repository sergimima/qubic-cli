#include <cstdint>
#include <cstring>
#include <stdexcept>

#include "structs.h"
#include "walletUtils.h"
#include "keyUtils.h"
#include "assetUtil.h"
#include "connection.h"
#include "logger.h"
#include "nodeUtils.h"
#include "K12AndKeyUtil.h"
#include "vottunBridge.h"

#define VOTTUNBRIDGE_CONTRACT_INDEX 13

// VOTTUNBRIDGE FUNCTIONS

#define VOTTUNBRIDGE_TYPE_GET_ORDER 1
#define VOTTUNBRIDGE_TYPE_GET_TOTAL_RECEIVED_TOKEN 4
#define VOTTUNBRIDGE_TYPE_GET_ADMIN_ID 5
#define VOTTUNBRIDGE_TYPE_GET_TOTAL_LOCKED_TOKEN 6
#define VOTTUNBRIDGE_TYPE_GET_ORDER_BY_DETAILS 7

// VOTTUNBRIDGE PROCEDURES

#define VOTTUNBRIDGE_TYPE_CREATE_ORDER 1
#define VOTTUNBRIDGE_TYPE_SET_ADMIN 2
#define VOTTUNBRIDGE_TYPE_ADD_MANAGER 3
#define VOTTUNBRIDGE_TYPE_REMOVE_MANAGER 4
#define VOTTUNBRIDGE_TYPE_COMPLETE_ORDER 5
#define VOTTUNBRIDGE_TYPE_REFUND_ORDER 6
#define VOTTUNBRIDGE_TYPE_TRANSFER_TO_CONTRACT 7

constexpr uint64_t TRANSACTION_FEE = 1000;

struct createOrder_input
{
    uint8_t ethAddress[32];
    uint64_t amount;
    bool fromQubicToEthereum;
};

struct createOrder_output
{
    uint8_t status;
    uint64_t orderId;
};

struct setAdmin_input
{
    uint8_t address[32];
};

struct setAdmin_output
{
    uint8_t status;
};

struct addManager_input
{
    uint8_t address[32]; 
};

struct addManager_output
{
    uint8_t status;
};

struct removeManager_input
{
    uint8_t address[32];
};

struct removeManager_output
{
    uint8_t status;
};

struct completeOrder_input
{
    uint64_t orderId;
};

struct completeOrder_output
{
    uint8_t status;
};

struct refundOrder_input
{
    uint64_t orderId;
};

struct refundOrder_output
{
    uint8_t status;
};

struct transferToContract_input
{
    uint64_t amount;
};

struct transferToContract_output
{
    uint8_t status;
};

void createOrder(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, const char* ethAddress, uint64_t amount, bool fromQubicToEthereum)
{
    auto qc = make_qc(nodeIp, nodePort);

    uint8_t publicKey[32] = {0};
    getPublicKeyFromIdentity(ethAddress, publicKey);

    uint8_t privateKey[32] = {0};
    uint8_t sourcePublicKey[32] = {0};  
    uint8_t destPublicKey[32] = {0};
    uint8_t subseed[32] = {0};
    uint8_t digest[32] = {0};
    uint8_t signature[64] = {0};
    char publicIdentity[128] = {0};
    char txHash[128] = {0};
    getSubseedFromSeed((uint8_t*)seed, subseed);
    getPrivateKeyFromSubSeed(subseed, privateKey);
    getPublicKeyFromPrivateKey(privateKey, sourcePublicKey);
    const bool isLowerCase = false;
    getIdentityFromPublicKey(sourcePublicKey, publicIdentity, isLowerCase);
    ((uint64_t*)destPublicKey)[0] = VOTTUNBRIDGE_CONTRACT_INDEX;
    ((uint64_t*)destPublicKey)[1] = 0;
    ((uint64_t*)destPublicKey)[2] = 0;
    ((uint64_t*)destPublicKey)[3] = 0;

    struct {
        RequestResponseHeader header;
        Transaction transaction;
        createOrder_input input;
        unsigned char signature[64];
    } packet;

    memcpy(packet.input.ethAddress, publicKey, 32);
    packet.input.amount = amount;
    packet.input.fromQubicToEthereum = fromQubicToEthereum;

    packet.transaction.amount = TRANSACTION_FEE;
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + scheduledTickOffset;
    packet.transaction.inputType = VOTTUNBRIDGE_TYPE_CREATE_ORDER;
    packet.transaction.inputSize = sizeof(createOrder_input);
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(createOrder_input),
                   digest,
                   32);
    sign(subseed, sourcePublicKey, digest, signature);
    memcpy(packet.signature, signature, 64);
    packet.header.setSize(sizeof(packet));
    packet.header.zeroDejavu();
    packet.header.setType(BROADCAST_TRANSACTION);
    qc->sendData((uint8_t *) &packet, packet.header.size());
    
    // Calcular el hash de la transacción
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(createOrder_input) + SIGNATURE_SIZE,
                   digest,
                   32); // recalcular digest para txhash
    getTxHashFromDigest(digest, txHash);
    
    // Mostrar información de la transacción
    LOG("createOrder tx has been sent!\n");
    printReceipt(packet.transaction, txHash, nullptr);
    LOG("run ./qubic-cli [...] -checktxontick %u %s\n", currentTick + scheduledTickOffset, txHash);
    LOG("to check your tx confirmation status\n");
    
    // Recibir y mostrar la respuesta del contrato
    struct {
        RequestResponseHeader header;
        createOrder_output output;
    } response;

    if (qc->receiveData((uint8_t*)&response, sizeof(response))) {
        if (response.output.status == 0) { // Éxito
            LOG("Order created successfully! Order ID: %llu\n", response.output.orderId);
        } else {
            LOG("Error creating order. Status: %u\n", response.output.status);
        }
    } else {
        LOG("Failed to receive response from contract\n");
    }
}

void setAdmin(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, const char* identity)
{
    auto qc = make_qc(nodeIp, nodePort);

    uint8_t publicKey[32] = {0};
    getPublicKeyFromIdentity(identity, publicKey);

    uint8_t privateKey[32] = {0};
    uint8_t sourcePublicKey[32] = {0};  
    uint8_t destPublicKey[32] = {0};
    uint8_t subseed[32] = {0};
    uint8_t digest[32] = {0};
    uint8_t signature[64] = {0};
    char publicIdentity[128] = {0};
    char txHash[128] = {0};
    getSubseedFromSeed((uint8_t*)seed, subseed);
    getPrivateKeyFromSubSeed(subseed, privateKey);
    getPublicKeyFromPrivateKey(privateKey, sourcePublicKey);
    const bool isLowerCase = false;
    getIdentityFromPublicKey(sourcePublicKey, publicIdentity, isLowerCase);
    ((uint64_t*)destPublicKey)[0] = VOTTUNBRIDGE_CONTRACT_INDEX;
    ((uint64_t*)destPublicKey)[1] = 0;
    ((uint64_t*)destPublicKey)[2] = 0;
    ((uint64_t*)destPublicKey)[3] = 0;

    struct {
        RequestResponseHeader header;
        Transaction transaction;
        setAdmin_input input;
        unsigned char signature[64];
    } packet;

    memcpy(packet.input.address, publicKey, 32);

    packet.transaction.amount = 0;
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + scheduledTickOffset;
    packet.transaction.inputType = VOTTUNBRIDGE_TYPE_SET_ADMIN;
    packet.transaction.inputSize = sizeof(setAdmin_input);
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(setAdmin_input),
                   digest,
                   32);
    sign(subseed, sourcePublicKey, digest, signature);
    memcpy(packet.signature, signature, 64);
    packet.header.setSize(sizeof(packet));
    packet.header.zeroDejavu();
    packet.header.setType(BROADCAST_TRANSACTION);
    qc->sendData((uint8_t *) &packet, packet.header.size());
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(setAdmin_input) + SIGNATURE_SIZE,
                   digest,
                   32); // recompute digest for txhash
    getTxHashFromDigest(digest, txHash);
    LOG("setAdmin tx has been sent!\n");
    printReceipt(packet.transaction, txHash, nullptr);
    LOG("run ./qubic-cli [...] -checktxontick %u %s\n", currentTick + scheduledTickOffset, txHash);
    LOG("to check your tx confirmation status\n");
}

void addManager(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, const char* identity)
{
    auto qc = make_qc(nodeIp, nodePort);

    uint8_t publicKey[32] = {0};
    getPublicKeyFromIdentity(identity, publicKey);

    uint8_t privateKey[32] = {0};
    uint8_t sourcePublicKey[32] = {0};  
    uint8_t destPublicKey[32] = {0};
    uint8_t subseed[32] = {0};
    uint8_t digest[32] = {0};
    uint8_t signature[64] = {0};
    char publicIdentity[128] = {0};
    char txHash[128] = {0};
    getSubseedFromSeed((uint8_t*)seed, subseed);
    getPrivateKeyFromSubSeed(subseed, privateKey);
    getPublicKeyFromPrivateKey(privateKey, sourcePublicKey);
    const bool isLowerCase = false;
    getIdentityFromPublicKey(sourcePublicKey, publicIdentity, isLowerCase);
    ((uint64_t*)destPublicKey)[0] = VOTTUNBRIDGE_CONTRACT_INDEX;
    ((uint64_t*)destPublicKey)[1] = 0;
    ((uint64_t*)destPublicKey)[2] = 0;
    ((uint64_t*)destPublicKey)[3] = 0;

    struct {
        RequestResponseHeader header;
        Transaction transaction;
        addManager_input input;
        unsigned char signature[64];
    } packet;

    memcpy(packet.input.address, publicKey, 32);

    packet.transaction.amount = 0;
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + scheduledTickOffset;
    packet.transaction.inputType = VOTTUNBRIDGE_TYPE_ADD_MANAGER;
    packet.transaction.inputSize = sizeof(addManager_input);
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(addManager_input),
                   digest,
                   32);
    sign(subseed, sourcePublicKey, digest, signature);
    memcpy(packet.signature, signature, 64);
    packet.header.setSize(sizeof(packet));
    packet.header.zeroDejavu();
    packet.header.setType(BROADCAST_TRANSACTION);
    qc->sendData((uint8_t *) &packet, packet.header.size());
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(addManager_input) + SIGNATURE_SIZE,
                   digest,
                   32); // recompute digest for txhash
    getTxHashFromDigest(digest, txHash);
    LOG("addManager tx has been sent!\n");
    printReceipt(packet.transaction, txHash, nullptr);
    LOG("run ./qubic-cli [...] -checktxontick %u %s\n", currentTick + scheduledTickOffset, txHash);
    LOG("to check your tx confirmation status\n");
}

void removeManager(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, const char* identity)
{
    auto qc = make_qc(nodeIp, nodePort);

    uint8_t publicKey[32] = {0};
    getPublicKeyFromIdentity(identity, publicKey);

    uint8_t privateKey[32] = {0};
    uint8_t sourcePublicKey[32] = {0};  
    uint8_t destPublicKey[32] = {0};
    uint8_t subseed[32] = {0};
    uint8_t digest[32] = {0};
    uint8_t signature[64] = {0};
    char publicIdentity[128] = {0};
    char txHash[128] = {0};
    getSubseedFromSeed((uint8_t*)seed, subseed);
    getPrivateKeyFromSubSeed(subseed, privateKey);
    getPublicKeyFromPrivateKey(privateKey, sourcePublicKey);
    const bool isLowerCase = false;
    getIdentityFromPublicKey(sourcePublicKey, publicIdentity, isLowerCase);
    ((uint64_t*)destPublicKey)[0] = VOTTUNBRIDGE_CONTRACT_INDEX;
    ((uint64_t*)destPublicKey)[1] = 0;
    ((uint64_t*)destPublicKey)[2] = 0;
    ((uint64_t*)destPublicKey)[3] = 0;

    struct {
        RequestResponseHeader header;
        Transaction transaction;
        removeManager_input input;
        unsigned char signature[64];
    } packet;

    memcpy(packet.input.address, publicKey, 32);

    packet.transaction.amount = 0;
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + scheduledTickOffset;
    packet.transaction.inputType = VOTTUNBRIDGE_TYPE_REMOVE_MANAGER;
    packet.transaction.inputSize = sizeof(removeManager_input);
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(removeManager_input),
                   digest,
                   32);
    sign(subseed, sourcePublicKey, digest, signature);
    memcpy(packet.signature, signature, 64);
    packet.header.setSize(sizeof(packet));
    packet.header.zeroDejavu();
    packet.header.setType(BROADCAST_TRANSACTION);
    qc->sendData((uint8_t *) &packet, packet.header.size());
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(removeManager_input) + SIGNATURE_SIZE,
                   digest,
                   32); // recompute digest for txhash
    getTxHashFromDigest(digest, txHash);
    LOG("removeManager tx has been sent!\n");
    printReceipt(packet.transaction, txHash, nullptr);
    LOG("run ./qubic-cli [...] -checktxontick %u %s\n", currentTick + scheduledTickOffset, txHash);
    LOG("to check your tx confirmation status\n");
}

void completeOrder(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, uint64_t orderId)
{
    auto qc = make_qc(nodeIp, nodePort);

    uint8_t privateKey[32] = {0};
    uint8_t sourcePublicKey[32] = {0};  
    uint8_t destPublicKey[32] = {0};
    uint8_t subseed[32] = {0};
    uint8_t digest[32] = {0};
    uint8_t signature[64] = {0};
    char publicIdentity[128] = {0};
    char txHash[128] = {0};
    getSubseedFromSeed((uint8_t*)seed, subseed);
    getPrivateKeyFromSubSeed(subseed, privateKey);
    getPublicKeyFromPrivateKey(privateKey, sourcePublicKey);
    const bool isLowerCase = false;
    getIdentityFromPublicKey(sourcePublicKey, publicIdentity, isLowerCase);
    ((uint64_t*)destPublicKey)[0] = VOTTUNBRIDGE_CONTRACT_INDEX;
    ((uint64_t*)destPublicKey)[1] = 0;
    ((uint64_t*)destPublicKey)[2] = 0;
    ((uint64_t*)destPublicKey)[3] = 0;

    struct {
        RequestResponseHeader header;
        Transaction transaction;
        completeOrder_input input;
        unsigned char signature[64];
    } packet;

    packet.input.orderId = orderId;

    packet.transaction.amount = 0;
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + scheduledTickOffset;
    packet.transaction.inputType = VOTTUNBRIDGE_TYPE_COMPLETE_ORDER;
    packet.transaction.inputSize = sizeof(completeOrder_input);
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(completeOrder_input),
                   digest,
                   32);
    sign(subseed, sourcePublicKey, digest, signature);
    memcpy(packet.signature, signature, 64);
    packet.header.setSize(sizeof(packet));
    packet.header.zeroDejavu();
    packet.header.setType(BROADCAST_TRANSACTION);
    qc->sendData((uint8_t *) &packet, packet.header.size());
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(completeOrder_input) + SIGNATURE_SIZE,
                   digest,
                   32); // recompute digest for txhash
    getTxHashFromDigest(digest, txHash);
    LOG("completeOrder tx has been sent!\n");
    printReceipt(packet.transaction, txHash, nullptr);
    LOG("run ./qubic-cli [...] -checktxontick %u %s\n", currentTick + scheduledTickOffset, txHash);
    LOG("to check your tx confirmation status\n");
}

void refundOrder(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, uint64_t orderId)
{
    auto qc = make_qc(nodeIp, nodePort);

    uint8_t privateKey[32] = {0};
    uint8_t sourcePublicKey[32] = {0};  
    uint8_t destPublicKey[32] = {0};
    uint8_t subseed[32] = {0};
    uint8_t digest[32] = {0};
    uint8_t signature[64] = {0};
    char publicIdentity[128] = {0};
    char txHash[128] = {0};
    getSubseedFromSeed((uint8_t*)seed, subseed);
    getPrivateKeyFromSubSeed(subseed, privateKey);
    getPublicKeyFromPrivateKey(privateKey, sourcePublicKey);
    const bool isLowerCase = false;
    getIdentityFromPublicKey(sourcePublicKey, publicIdentity, isLowerCase);
    ((uint64_t*)destPublicKey)[0] = VOTTUNBRIDGE_CONTRACT_INDEX;
    ((uint64_t*)destPublicKey)[1] = 0;
    ((uint64_t*)destPublicKey)[2] = 0;
    ((uint64_t*)destPublicKey)[3] = 0;

    struct {
        RequestResponseHeader header;
        Transaction transaction;
        refundOrder_input input;
        unsigned char signature[64];
    } packet;

    packet.input.orderId = orderId;

    packet.transaction.amount = 0;
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + scheduledTickOffset;
    packet.transaction.inputType = VOTTUNBRIDGE_TYPE_REFUND_ORDER;
    packet.transaction.inputSize = sizeof(refundOrder_input);
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(refundOrder_input),
                   digest,
                   32);
    sign(subseed, sourcePublicKey, digest, signature);
    memcpy(packet.signature, signature, 64);
    packet.header.setSize(sizeof(packet));
    packet.header.zeroDejavu();
    packet.header.setType(BROADCAST_TRANSACTION);
    qc->sendData((uint8_t *) &packet, packet.header.size());
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(refundOrder_input) + SIGNATURE_SIZE,
                   digest,
                   32); // recompute digest for txhash
    getTxHashFromDigest(digest, txHash);
    LOG("refundOrder tx has been sent!\n");
    printReceipt(packet.transaction, txHash, nullptr);
    LOG("run ./qubic-cli [...] -checktxontick %u %s\n", currentTick + scheduledTickOffset, txHash);
    LOG("to check your tx confirmation status\n");
}

void transferToContract(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, uint64_t amount)
{
    auto qc = make_qc(nodeIp, nodePort);

    uint8_t privateKey[32] = {0};
    uint8_t sourcePublicKey[32] = {0};  
    uint8_t destPublicKey[32] = {0};
    uint8_t subseed[32] = {0};
    uint8_t digest[32] = {0};
    uint8_t signature[64] = {0};
    char publicIdentity[128] = {0};
    char txHash[128] = {0};
    getSubseedFromSeed((uint8_t*)seed, subseed);
    getPrivateKeyFromSubSeed(subseed, privateKey);
    getPublicKeyFromPrivateKey(privateKey, sourcePublicKey);
    const bool isLowerCase = false;
    getIdentityFromPublicKey(sourcePublicKey, publicIdentity, isLowerCase);
    ((uint64_t*)destPublicKey)[0] = VOTTUNBRIDGE_CONTRACT_INDEX;
    ((uint64_t*)destPublicKey)[1] = 0;
    ((uint64_t*)destPublicKey)[2] = 0;
    ((uint64_t*)destPublicKey)[3] = 0;

    struct {
        RequestResponseHeader header;
        Transaction transaction;
        transferToContract_input input;
        unsigned char signature[64];
    } packet;

    packet.input.amount = amount;

    packet.transaction.amount = 0;
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + scheduledTickOffset;
    packet.transaction.inputType = VOTTUNBRIDGE_TYPE_TRANSFER_TO_CONTRACT;
    packet.transaction.inputSize = sizeof(transferToContract_input);
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(transferToContract_input),
                   digest,
                   32);
    sign(subseed, sourcePublicKey, digest, signature);
    memcpy(packet.signature, signature, 64);
    packet.header.setSize(sizeof(packet));
    packet.header.zeroDejavu();
    packet.header.setType(BROADCAST_TRANSACTION);
    qc->sendData((uint8_t *) &packet, packet.header.size());
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(transferToContract_input) + SIGNATURE_SIZE,
                   digest,
                   32); // recompute digest for txhash
    getTxHashFromDigest(digest, txHash);
    LOG("transferToContract tx has been sent!\n");
    printReceipt(packet.transaction, txHash, nullptr);
    LOG("run ./qubic-cli [...] -checktxontick %u %s\n", currentTick + scheduledTickOffset, txHash);
    LOG("to check your tx confirmation status\n");
}

void getOrder(const char* nodeIp, int nodePort, uint64_t orderId)
{
    auto qc = make_qc(nodeIp, nodePort);
    
    struct {
        RequestResponseHeader header;
        RequestContractFunction rcf;
        vottunBridgeGetOrder_input input;
    } packet;
    packet.header.setSize(sizeof(packet));
    packet.header.randomizeDejavu();
    packet.header.setType(RequestContractFunction::type());
    packet.rcf.inputSize = sizeof(vottunBridgeGetOrder_input);
    packet.rcf.inputType = VOTTUNBRIDGE_TYPE_GET_ORDER;
    packet.rcf.contractIndex = VOTTUNBRIDGE_CONTRACT_INDEX;
    packet.input.orderId = orderId;
    
    qc->sendData((uint8_t *) &packet, packet.header.size());

    vottunBridgeGetOrder_output result;
    try
    {
        result = qc->receivePacketWithHeaderAs<vottunBridgeGetOrder_output>();
    }
    catch (std::logic_error)
    {
        LOG("Failed to receive data\n");
        return;
    }

    char originAccount[128] = {0};
    char destinationAccount[128] = {0};
    getIdentityFromPublicKey(result.order.originAccount, originAccount, false);
    getIdentityFromPublicKey(result.order.destinationAccount, destinationAccount, false);

    printf("The status of Order%u\n\n", result.status);
    printf("%s\nOrderId: %llu\nOriginAccount: %s\nDestinationAccount: %s\nAmount: %lld\nMetadata: %s\nSource Chain: %u", result.message,result.order.orderId, originAccount, destinationAccount, result.order.amount, result.order.memo, result.order.sourceChain);
}

void getTotalReceivedTokens(const char* nodeIp, int nodePort, uint64_t amount)
{
    auto qc = make_qc(nodeIp, nodePort);
    
    struct {
        RequestResponseHeader header;
        RequestContractFunction rcf;
        vottunBridgeGetTotalReceivedTokens_input input;
    } packet;
    packet.header.setSize(sizeof(packet));
    packet.header.randomizeDejavu();
    packet.header.setType(RequestContractFunction::type());
    packet.rcf.inputSize = sizeof(vottunBridgeGetTotalReceivedTokens_input);
    packet.rcf.inputType = VOTTUNBRIDGE_TYPE_GET_TOTAL_RECEIVED_TOKEN;
    packet.rcf.contractIndex = VOTTUNBRIDGE_CONTRACT_INDEX;
    packet.input.amount = amount;
    
    qc->sendData((uint8_t *) &packet, packet.header.size());

    vottunBridgeGetTotalReceivedTokens_output result;
    try
    {
        result = qc->receivePacketWithHeaderAs<vottunBridgeGetTotalReceivedTokens_output>();
    }
    catch (std::logic_error)
    {
        LOG("Failed to receive data\n");
        return;
    }

    printf("Total Received Token: %lld\n", result.totalTokens);
}

void getAdminID(const char* nodeIp, int nodePort, uint8_t idInput)
{
    auto qc = make_qc(nodeIp, nodePort);
    
    struct {
        RequestResponseHeader header;
        RequestContractFunction rcf;
        vottunBridgeGetAdminID_input input;
    } packet;
    packet.header.setSize(sizeof(packet));
    packet.header.randomizeDejavu();
    packet.header.setType(RequestContractFunction::type());
    packet.rcf.inputSize = sizeof(vottunBridgeGetAdminID_input);
    packet.rcf.inputType = VOTTUNBRIDGE_TYPE_GET_ADMIN_ID;
    packet.rcf.contractIndex = VOTTUNBRIDGE_CONTRACT_INDEX;
    packet.input.idInput = idInput;
    
    qc->sendData((uint8_t *) &packet, packet.header.size());

    vottunBridgeGetAdminID_output result;
    try
    {
        result = qc->receivePacketWithHeaderAs<vottunBridgeGetAdminID_output>();
    }
    catch (std::logic_error)
    {
        LOG("Failed to receive data\n");
        return;
    }

    char adminId[128] = {0};
    getIdentityFromPublicKey(result.adminId, adminId, false);

    printf("Admin Id: %s\n", adminId);
}

void getTotalLockedTokens(const char* nodeIp, int nodePort)
{
    auto qc = make_qc(nodeIp, nodePort);
    
    struct {
        RequestResponseHeader header;
        RequestContractFunction rcf;
    } packet;
    packet.header.setSize(sizeof(packet));
    packet.header.randomizeDejavu();
    packet.header.setType(RequestContractFunction::type());
    packet.rcf.inputSize = 0;
    packet.rcf.inputType = VOTTUNBRIDGE_TYPE_GET_TOTAL_LOCKED_TOKEN;
    packet.rcf.contractIndex = VOTTUNBRIDGE_CONTRACT_INDEX;
    
    qc->sendData((uint8_t *) &packet, packet.header.size());

    vottunBridgeGetTotalLockedTokens_output result;
    try
    {
        result = qc->receivePacketWithHeaderAs<vottunBridgeGetTotalLockedTokens_output>();
    }
    catch (std::logic_error)
    {
        LOG("Failed to receive data\n");
        return;
    }

    printf("Total Locked Token: %lld\n", result.totalLockedTokens);
}

void getOrderByDetails(const char* nodeIp, int nodePort, const char* ethAddress, uint64_t amount, uint8_t status)
{
    auto qc = make_qc(nodeIp, nodePort);

    uint8_t publicKey[32] = {0};
    getPublicKeyFromIdentity(ethAddress, publicKey);
    
    struct {
        RequestResponseHeader header;
        RequestContractFunction rcf;
        vottunBridgeGetOrderByDetails_input input;
    } packet;
    packet.header.setSize(sizeof(packet));
    packet.header.randomizeDejavu();
    packet.header.setType(RequestContractFunction::type());
    packet.rcf.inputSize = sizeof(vottunBridgeGetOrderByDetails_input);
    packet.rcf.inputType = VOTTUNBRIDGE_TYPE_GET_ORDER_BY_DETAILS;
    packet.rcf.contractIndex = VOTTUNBRIDGE_CONTRACT_INDEX;
    packet.input.amount = amount;
    packet.input.status = status;

    memcpy(packet.input.ethAddress, publicKey, 32);
    
    qc->sendData((uint8_t *) &packet, packet.header.size());

    vottunBridgeGetOrderByDetails_output result;
    try
    {
        result = qc->receivePacketWithHeaderAs<vottunBridgeGetOrderByDetails_output>();
    }
    catch (std::logic_error)
    {
        LOG("Failed to receive data\n");
        return;
    }

}
