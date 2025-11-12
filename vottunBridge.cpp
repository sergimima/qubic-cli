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

#define VOTTUNBRIDGE_CONTRACT_INDEX 18

// VOTTUNBRIDGE FUNCTIONS

#define VOTTUNBRIDGE_TYPE_GET_ORDER 1
#define VOTTUNBRIDGE_TYPE_IS_MANAGER 2
#define VOTTUNBRIDGE_TYPE_GET_TOTAL_RECEIVED_TOKEN 3
#define VOTTUNBRIDGE_TYPE_GET_TOTAL_LOCKED_TOKEN 4
#define VOTTUNBRIDGE_TYPE_GET_ORDER_BY_DETAILS 5
#define VOTTUNBRIDGE_TYPE_GET_CONTRACT_INFO 6
#define VOTTUNBRIDGE_TYPE_GET_AVAILABLE_FEES 7
#define VOTTUNBRIDGE_TYPE_GET_PROPOSAL 8

// VOTTUNBRIDGE PROCEDURES

#define VOTTUNBRIDGE_TYPE_CREATE_ORDER 1
#define VOTTUNBRIDGE_TYPE_ADD_MANAGER 2
#define VOTTUNBRIDGE_TYPE_REMOVE_MANAGER 3
#define VOTTUNBRIDGE_TYPE_COMPLETE_ORDER 4
#define VOTTUNBRIDGE_TYPE_REFUND_ORDER 5
#define VOTTUNBRIDGE_TYPE_TRANSFER_TO_CONTRACT 6
#define VOTTUNBRIDGE_TYPE_WITHDRAW_FEES 7
#define VOTTUNBRIDGE_TYPE_ADD_LIQUIDITY 8
#define VOTTUNBRIDGE_TYPE_CREATE_PROPOSAL 9
#define VOTTUNBRIDGE_TYPE_APPROVE_PROPOSAL 10

constexpr uint64_t TRANSACTION_FEE = 1000;

struct createOrder_input
{
    uint8_t qubicDestination[32];
    uint64_t amount;
    uint8_t ethAddress[64];
    bool fromQubicToEthereum;
};

struct createOrder_output
{
    uint8_t status;
    uint64_t orderId;
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
    uint64_t orderId;
};

struct transferToContract_output
{
    uint8_t status;
};

struct withdrawFees_input
{
    uint64_t amount;
};

struct withdrawFees_output
{
    uint8_t status;
};

struct addLiquidity_input
{
};

struct addLiquidity_output
{
    uint8_t status;
    uint64_t addedAmount;
    uint64_t totalLocked;
};

void createOrder(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, const char* qubicDestination, const char* ethAddress, uint64_t amount, bool fromQubicToEthereum)
{
    auto qc = make_qc(nodeIp, nodePort);

    uint8_t publicKey[32] = {0};
    getPublicKeyFromIdentity(qubicDestination, publicKey);

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

    #pragma pack(push, 1)
    struct {
        RequestResponseHeader header;
        Transaction transaction;
        createOrder_input input;
        unsigned char signature[64];
    } packet;
    #pragma pack(pop)

    memcpy(packet.input.qubicDestination, publicKey, 32);
    packet.input.amount = amount;
    memcpy(packet.input.ethAddress, ethAddress, 64);
    packet.input.fromQubicToEthereum = fromQubicToEthereum;

    // Calculate fee: 0.5% (5000000 billionths)
    uint64_t tradeFeeBillionths = 5000000;
    uint64_t requiredFeeEth = (amount * tradeFeeBillionths) / 1000000000;
    uint64_t requiredFeeQubic = (amount * tradeFeeBillionths) / 1000000000;
    uint64_t totalRequiredFee = requiredFeeEth + requiredFeeQubic;

    printf("DEBUG: amount=%llu, feeEth=%llu, feeQubic=%llu, totalFee=%llu\n",
           (unsigned long long)amount, (unsigned long long)requiredFeeEth,
           (unsigned long long)requiredFeeQubic, (unsigned long long)totalRequiredFee);

    packet.transaction.amount = totalRequiredFee;
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

    #pragma pack(push, 1)
    struct {
        RequestResponseHeader header;
        Transaction transaction;
        addManager_input input;
        unsigned char signature[64];
    } packet;
    #pragma pack(pop)

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

    #pragma pack(push, 1)
    struct {
        RequestResponseHeader header;
        Transaction transaction;
        removeManager_input input;
        unsigned char signature[64];
    } packet;
    #pragma pack(pop)

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

    #pragma pack(push, 1)
    struct {
        RequestResponseHeader header;
        Transaction transaction;
        completeOrder_input input;
        unsigned char signature[64];
    } packet;
    #pragma pack(pop)

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

    #pragma pack(push, 1)
    struct {
        RequestResponseHeader header;
        Transaction transaction;
        refundOrder_input input;
        unsigned char signature[64];
    } packet;
    #pragma pack(pop)

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

void transferToContract(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, uint64_t amount, uint64_t orderId)
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

    #pragma pack(push, 1)
    struct {
        RequestResponseHeader header;
        Transaction transaction;
        transferToContract_input input;
        unsigned char signature[64];
    } packet;
    #pragma pack(pop)

    packet.input.amount = amount;
    packet.input.orderId = orderId;

    packet.transaction.amount = amount;
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

void addLiquidity(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, uint64_t amount)
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

    #pragma pack(push, 1)
    struct {
        RequestResponseHeader header;
        Transaction transaction;
        addLiquidity_input input;
        unsigned char signature[64];
    } packet;
    #pragma pack(pop)

    packet.transaction.amount = amount;
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + scheduledTickOffset;
    packet.transaction.inputType = VOTTUNBRIDGE_TYPE_ADD_LIQUIDITY;
    packet.transaction.inputSize = sizeof(addLiquidity_input);
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(addLiquidity_input),
                   digest,
                   32);
    sign(subseed, sourcePublicKey, digest, signature);
    memcpy(packet.signature, signature, 64);
    packet.header.setSize(sizeof(packet));
    packet.header.zeroDejavu();
    packet.header.setType(BROADCAST_TRANSACTION);
    qc->sendData((uint8_t *) &packet, packet.header.size());
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(addLiquidity_input) + SIGNATURE_SIZE,
                   digest,
                   32); // recompute digest for txhash
    getTxHashFromDigest(digest, txHash);
    LOG("addLiquidity tx has been sent!\n");
    printReceipt(packet.transaction, txHash, nullptr);
    LOG("run ./qubic-cli [...] -checktxontick %u %s\n", currentTick + scheduledTickOffset, txHash);
    LOG("to check your tx confirmation status\n");
}

void withdrawFees(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, uint64_t amount)
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

    #pragma pack(push, 1)
    struct {
        RequestResponseHeader header;
        Transaction transaction;
        withdrawFees_input input;
        unsigned char signature[64];
    } packet;
    #pragma pack(pop)

    packet.input.amount = amount;
    packet.transaction.amount = 0;
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + scheduledTickOffset;
    packet.transaction.inputType = VOTTUNBRIDGE_TYPE_WITHDRAW_FEES;
    packet.transaction.inputSize = sizeof(withdrawFees_input);
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(withdrawFees_input),
                   digest,
                   32);
    sign(subseed, sourcePublicKey, digest, signature);
    memcpy(packet.signature, signature, 64);
    packet.header.setSize(sizeof(packet));
    packet.header.zeroDejavu();
    packet.header.setType(BROADCAST_TRANSACTION);
    qc->sendData((uint8_t *) &packet, packet.header.size());
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(withdrawFees_input) + SIGNATURE_SIZE,
                   digest,
                   32); // recompute digest for txhash
    getTxHashFromDigest(digest, txHash);
    LOG("withdrawFees tx has been sent!\n");
    printReceipt(packet.transaction, txHash, nullptr);
    LOG("run ./qubic-cli [...] -checktxontick %u %s\n", currentTick + scheduledTickOffset, txHash);
    LOG("to check your tx confirmation status\n");
}

void getOrder(const char* nodeIp, int nodePort, uint64_t orderId)
{
    auto qc = make_qc(nodeIp, nodePort);
    
    #pragma pack(push, 1)
    struct {
        RequestResponseHeader header;
        RequestContractFunction rcf;
        vottunBridgeGetOrder_input input;
    } packet;
    #pragma pack(pop)

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

    printf("The status of Order%llu is %u\n\n", orderId, result.order.status);
    printf("%s\nOrderId: %llu\nOriginAccount: %s\nDestinationAccount: %s\nAmount: %llu\nMetadata: %s\nSource Chain: %u", result.message,result.order.orderId, originAccount, destinationAccount, (unsigned long long)result.order.amount, result.order.memo, result.order.sourceChain);
}

void getTotalReceivedTokens(const char* nodeIp, int nodePort, uint64_t amount)
{
    auto qc = make_qc(nodeIp, nodePort);
    
    #pragma pack(push, 1)
    struct {
        RequestResponseHeader header;
        RequestContractFunction rcf;
        vottunBridgeGetTotalReceivedTokens_input input;
    } packet;
    #pragma pack(pop)
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

    printf("Total Received Token: %llu\n", (unsigned long long)result.totalTokens);
}

void getTotalLockedTokens(const char* nodeIp, int nodePort)
{
    auto qc = make_qc(nodeIp, nodePort);
    
    #pragma pack(push, 1)
    struct {
        RequestResponseHeader header;
        RequestContractFunction rcf;
    } packet;
    #pragma pack(pop)
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

    printf("Total Locked Token: %llu\n", (unsigned long long)result.totalLockedTokens);
}

void getOrderByDetails(const char* nodeIp, int nodePort, const char* ethAddress, uint64_t amount, uint8_t status)
{
    auto qc = make_qc(nodeIp, nodePort);

    uint8_t publicKey[32] = {0};
    getPublicKeyFromIdentity(ethAddress, publicKey);
    
    #pragma pack(push, 1)
    struct {
        RequestResponseHeader header;
        RequestContractFunction rcf;
        vottunBridgeGetOrderByDetails_input input;
    } packet;
    #pragma pack(pop)
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

    if (result.status)
    {
        printf("There is no the order%llu", result.orderId);
    }
    else 
    {
        printf("The order%llu exists", result.orderId);
    }

    char qubicDestination[128] = {0};
    getIdentityFromPublicKey(result.qubicDestination, qubicDestination, false);
    printf("Qubic Destination: %s\n", qubicDestination);
}

void getContractInfo(const char* nodeIp, int nodePort)
{
    auto qc = make_qc(nodeIp, nodePort);

    #pragma pack(push, 1)
    struct {
        RequestResponseHeader header;
        RequestContractFunction rcf;
        vottunBridgeGetContractInfo_input input;
    } packet;
    #pragma pack(pop)
    packet.header.setSize(sizeof(packet));
    packet.header.randomizeDejavu();
    packet.header.setType(RequestContractFunction::type());
    packet.rcf.inputSize = sizeof(vottunBridgeGetContractInfo_input);
    packet.rcf.inputType = VOTTUNBRIDGE_TYPE_GET_CONTRACT_INFO;
    packet.rcf.contractIndex = VOTTUNBRIDGE_CONTRACT_INDEX;
    
    qc->sendData((uint8_t *) &packet, packet.header.size());

    vottunBridgeGetContractInfo_output result;

    try
    {
        result = qc->receivePacketWithHeaderAs<vottunBridgeGetContractInfo_output>();
    }
    catch (std::logic_error)
    {
        LOG("Failed to receive data\n");
        return;
    }

    // Debug: Print first 544 bytes (up to sourceChain)
    printf("\nDEBUG: First 544 bytes received:\n");
    uint8_t* raw = (uint8_t*)&result;
    for (int i = 0; i < 544 && i < sizeof(result); i++) {
        printf("%02x ", raw[i]);
        if ((i + 1) % 32 == 0) printf("\n");
    }
    printf("\n");

    printf("=== Managers ===\n");
    for (int i = 0; i < 16; i++)
    {
        char manager[128] = {0};
        getIdentityFromPublicKey(result.managers[i], manager, false);
        if (manager[0] != '\0' && strcmp(manager, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAFXIB") != 0)
        {
            printf("Manager %d: %s\n", i, manager);
        }
    }

    printf("Next Order ID: %llu\n", result.nextOrderId);
    printf("Locked Tokens: %llu\n", result.lockedTokens);
    printf("Total Received Tokens: %llu\n", result.totalReceivedTokens);
    printf("Earned Fees: %llu\n", result.earnedFees);
    printf("Trade Fee Billionths: %u\n", result.tradeFeeBillionths);
    printf("Source Chain: %u\n", result.sourceChain);

    for (int i = 0; i < 16; i++)
    {
        printf("Order %d:\n", i);
        char qubicSender[128] = {0};
        getIdentityFromPublicKey(result.firstOrders[i].qubicSender, qubicSender, false);
        printf("Qubic Sender: %s\n", qubicSender);

        char qubicDestination[128] = {0};
        getIdentityFromPublicKey(result.firstOrders[i].qubicDestination, qubicDestination, false);
        printf("Qubic Destination: %s\n", qubicDestination);

        printf("Eth Address: ");
        for (int j = 0; j < 64; j++) {
            printf("%02x", result.firstOrders[i].ethAddress[j]);
        }
        printf("\n");

        printf("Order ID: %llu\n", result.firstOrders[i].orderId);
        printf("Amount: %llu\n", result.firstOrders[i].amount);
        printf("Order Type: %u\n", result.firstOrders[i].orderType);
        printf("Status: %u\n", result.firstOrders[i].status);
        printf("From Qubic To Ethereum: %u\n", result.firstOrders[i].fromQubicToEthereum);
        printf("Tokens Received: %u\n", result.firstOrders[i].tokensReceived);
        printf("Tokens Locked: %u\n\n", result.firstOrders[i].tokensLocked);
    }

    printf("Total Orders Found: %llu\n", result.totalOrdersFound);
    printf("Empty Slots: %llu\n", result.emptySlots);

    // Print multisig information
    printf("\n=== Multisig Configuration ===\n");
    printf("Number of Admins: %u\n", result.numberOfAdmins);
    printf("Required Approvals: %u\n", result.requiredApprovals);
    printf("Total Active Proposals: %llu\n", result.totalProposals);

    printf("\nMultisig Admins:\n");
    for (int i = 0; i < result.numberOfAdmins && i < 16; i++)
    {
        char multisigAdmin[128] = {0};
        getIdentityFromPublicKey(result.multisigAdmins[i], multisigAdmin, false);
        printf("  Admin %d: %s\n", i + 1, multisigAdmin);
    }
}

void getAvailableFees(const char* nodeIp, int nodePort)
{
    auto qc = make_qc(nodeIp, nodePort);

    #pragma pack(push, 1)
    struct {
        RequestResponseHeader header;
        RequestContractFunction rcf;
        vottunBridgeGetAvailableFees_input input;
    } packet;
    #pragma pack(pop)
    packet.header.setSize(sizeof(packet));
    packet.header.randomizeDejavu();
    packet.header.setType(RequestContractFunction::type());
    packet.rcf.inputSize = sizeof(vottunBridgeGetAvailableFees_input);
    packet.rcf.inputType = VOTTUNBRIDGE_TYPE_GET_AVAILABLE_FEES;
    packet.rcf.contractIndex = VOTTUNBRIDGE_CONTRACT_INDEX;
    
    qc->sendData((uint8_t *) &packet, packet.header.size());

    vottunBridgeGetAvailableFees_output result;
    try
    {
        result = qc->receivePacketWithHeaderAs<vottunBridgeGetAvailableFees_output>();
    }
    catch (std::logic_error)
    {
        LOG("Failed to receive data\n");
        return;
    }

    printf("Available Fees: %llu\n", result.availableFees);
    printf("Total Earned Fees: %llu\n", result.totalEarnedFees);
    printf("Total Distributed Fees: %llu\n", result.totalDistributedFees);
}

// Multisig Functions

void createProposal(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset,
                    uint8_t proposalType, const char* targetAddress, const char* oldAddress, uint64_t amount)
{
    auto qc = make_qc(nodeIp, nodePort);

    uint8_t targetPublicKey[32] = {0};
    if (targetAddress && strlen(targetAddress) > 0) {
        getPublicKeyFromIdentity(targetAddress, targetPublicKey);
    }

    uint8_t oldPublicKey[32] = {0};
    if (oldAddress && strlen(oldAddress) > 0) {
        getPublicKeyFromIdentity(oldAddress, oldPublicKey);
    }

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

    #pragma pack(push, 1)
    struct {
        RequestResponseHeader header;
        Transaction transaction;
        createProposal_input input;
        unsigned char signature[64];
    } packet;
    #pragma pack(pop)

    packet.input.proposalType = proposalType;
    memcpy(packet.input.targetAddress, targetPublicKey, 32);
    memcpy(packet.input.oldAddress, oldPublicKey, 32);
    packet.input.amount = amount;

    packet.transaction.amount = 0;
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + scheduledTickOffset;
    packet.transaction.inputType = VOTTUNBRIDGE_TYPE_CREATE_PROPOSAL;
    packet.transaction.inputSize = sizeof(createProposal_input);
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(createProposal_input),
                   digest,
                   32);
    sign(subseed, sourcePublicKey, digest, signature);
    memcpy(packet.signature, signature, 64);
    packet.header.setSize(sizeof(packet));
    packet.header.zeroDejavu();
    packet.header.setType(BROADCAST_TRANSACTION);
    qc->sendData((uint8_t *) &packet, packet.header.size());
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(createProposal_input) + SIGNATURE_SIZE,
                   digest,
                   32); // recompute digest for txhash
    getTxHashFromDigest(digest, txHash);
    LOG("createProposal tx has been sent!\n");
    LOG("Proposal Type: %u\n", proposalType);
    if (targetAddress && strlen(targetAddress) > 0) {
        LOG("Target Address: %s\n", targetAddress);
    }
    if (oldAddress && strlen(oldAddress) > 0) {
        LOG("Old Address: %s\n", oldAddress);
    }
    if (amount > 0) {
        LOG("Amount: %llu\n", amount);
    }
    printReceipt(packet.transaction, txHash, nullptr);
    LOG("run ./qubic-cli [...] -checktxontick %u %s\n", currentTick + scheduledTickOffset, txHash);
    LOG("to check your tx confirmation status\n");
}

void approveProposal(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, uint64_t proposalId)
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

    #pragma pack(push, 1)
    struct {
        RequestResponseHeader header;
        Transaction transaction;
        approveProposal_input input;
        unsigned char signature[64];
    } packet;
    #pragma pack(pop)

    packet.input.proposalId = proposalId;

    packet.transaction.amount = 0;
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + scheduledTickOffset;
    packet.transaction.inputType = VOTTUNBRIDGE_TYPE_APPROVE_PROPOSAL;
    packet.transaction.inputSize = sizeof(approveProposal_input);
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(approveProposal_input),
                   digest,
                   32);
    sign(subseed, sourcePublicKey, digest, signature);
    memcpy(packet.signature, signature, 64);
    packet.header.setSize(sizeof(packet));
    packet.header.zeroDejavu();
    packet.header.setType(BROADCAST_TRANSACTION);
    qc->sendData((uint8_t *) &packet, packet.header.size());
    KangarooTwelve((unsigned char*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(approveProposal_input) + SIGNATURE_SIZE,
                   digest,
                   32); // recompute digest for txhash
    getTxHashFromDigest(digest, txHash);
    LOG("approveProposal tx has been sent!\n");
    printReceipt(packet.transaction, txHash, nullptr);
    LOG("run ./qubic-cli [...] -checktxontick %u %s\n", currentTick + scheduledTickOffset, txHash);
    LOG("to check your tx confirmation status\n");
}

void getProposal(const char* nodeIp, int nodePort, uint64_t proposalId)
{
    auto qc = make_qc(nodeIp, nodePort);

    #pragma pack(push, 1)
    struct {
        RequestResponseHeader header;
        RequestContractFunction rcf;
        vottunBridgeGetProposal_input input;
    } packet;
    #pragma pack(pop)

    packet.header.setSize(sizeof(packet));
    packet.header.randomizeDejavu();
    packet.header.setType(RequestContractFunction::type());
    packet.rcf.inputSize = sizeof(vottunBridgeGetProposal_input);
    packet.rcf.inputType = VOTTUNBRIDGE_TYPE_GET_PROPOSAL;
    packet.rcf.contractIndex = VOTTUNBRIDGE_CONTRACT_INDEX;
    packet.input.proposalId = proposalId;

    qc->sendData((uint8_t *) &packet, packet.header.size());

    vottunBridgeGetProposal_output result;
    try
    {
        result = qc->receivePacketWithHeaderAs<vottunBridgeGetProposal_output>();
    }
    catch (std::logic_error)
    {
        LOG("Failed to receive data\n");
        return;
    }

    if (result.status != 0)
    {
        printf("Proposal not found or error status: %u\n", result.status);
        return;
    }

    printf("\n=== Proposal %llu ===\n", result.proposal.proposalId);
    printf("Type: %u\n", result.proposal.proposalType);
    printf("Amount: %llu\n", result.proposal.amount);
    printf("Approvals Count: %u\n", result.proposal.approvalsCount);
    printf("Executed: %s\n", result.proposal.executed ? "Yes" : "No");
    printf("Active: %s\n", result.proposal.active ? "Yes" : "No");

    char targetAddr[128] = {0};
    getIdentityFromPublicKey(result.proposal.targetAddress, targetAddr, false);
    printf("Target Address: %s\n", targetAddr);

    printf("\nApprovers:\n");
    for (int i = 0; i < result.proposal.approvalsCount && i < 16; i++)
    {
        char approver[128] = {0};
        getIdentityFromPublicKey(result.proposal.approvals[i], approver, false);
        printf("  %d. %s\n", i + 1, approver);
    }
}