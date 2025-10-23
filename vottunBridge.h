#pragma once

#include <cstdint>
#include "structs.h"

// SC structs

struct OrderResponse
{
    uint64_t orderId;        // Order ID as uint64
    uint8_t originAccount[32];      // Origin account
    uint8_t destinationAccount[32]; // Destination account
    uint64_t amount;         // Amount as uint64
    uint8_t memo[64]; // Notes or metadata
    uint32_t sourceChain;    // Source chain identifier
};

    struct vottunBridgeGetOrder_input
    {
        uint64_t orderId;
    };

struct vottunBridgeGetOrder_output
{
    uint8_t status;
    uint8_t message[32];
    OrderResponse order; // Updated response format

    static constexpr unsigned char type()
    {
        return RespondContractFunction::type();
    }
};

struct vottunBridgeGetTotalReceivedTokens_input
{
    uint64_t amount;
};

struct vottunBridgeGetTotalReceivedTokens_output
{
    uint64_t totalTokens;

    static constexpr unsigned char type()
    {
        return RespondContractFunction::type();
    }
};

struct vottunBridgeGetAdminID_input
{
    uint8_t idInput;
};

struct vottunBridgeGetAdminID_output
{
    uint8_t adminId[32];

    static constexpr unsigned char type()
    {
        return RespondContractFunction::type();
    }
};

struct vottunBridgeGetTotalLockedTokens_input
{
    // No input parameters
};

struct vottunBridgeGetTotalLockedTokens_output
{
    uint64_t totalLockedTokens;

    static constexpr unsigned char type()
    {
        return RespondContractFunction::type();
    }
};

// Estructura para la entrada de la función getOrderByDetails
struct vottunBridgeGetOrderByDetails_input
{
    uint8_t ethAddress[32];        // Dirección Ethereum
    uint64_t amount;        // Monto de la transacción
    uint8_t status;         // Estado de la orden (0 = creada, 1 = completada, 2 = reembolsada)
};

// Estructura para la salida de la función getOrderByDetails
struct vottunBridgeGetOrderByDetails_output
{
    uint8_t status;         // Estado de la operación (0 = éxito, otro = error)
    uint64_t orderId;       // ID de la orden encontrada
    uint8_t qubicDestination[32]; // Qubic destination public key

    static constexpr unsigned char type()
    {
        return RespondContractFunction::type();
    }
};

// Multisig proposal structure
struct AdminProposal
{
    uint64_t proposalId;
    uint8_t proposalType;
    uint8_t targetAddress[32];
    uint64_t amount;
    uint8_t approvals[16][32];
    uint8_t approvalsCount;
    uint8_t executed;
    uint8_t active;
};

struct createProposal_input
{
    uint8_t proposalType;
    uint8_t targetAddress[32];
    uint64_t amount;
};

struct createProposal_output
{
    uint8_t status;
    uint64_t proposalId;

    static constexpr unsigned char type()
    {
        return RespondContractFunction::type();
    }
};

struct approveProposal_input
{
    uint64_t proposalId;
};

struct approveProposal_output
{
    uint8_t status;
    uint8_t executed;

    static constexpr unsigned char type()
    {
        return RespondContractFunction::type();
    }
};

struct vottunBridgeGetProposal_input
{
    uint64_t proposalId;
};

struct vottunBridgeGetProposal_output
{
    uint8_t status;
    AdminProposal proposal;

    static constexpr unsigned char type()
    {
        return RespondContractFunction::type();
    }
};

// Procedure declarations
void createOrder(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, const char* qubicDestination, const char* ethAddress, uint64_t amount, bool fromQubicToEthereum);
void setAdmin(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, const char* identity);
void addManager(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, const char* identity);
void removeManager(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, const char* identity);
void completeOrder(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, uint64_t orderId);
void refundOrder(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, uint64_t orderId);
void transferToContract(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, uint64_t amount, uint64_t orderId);
void createProposal(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, uint8_t proposalType, const char* targetAddress, uint64_t amount);
void approveProposal(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, uint64_t proposalId);

struct vottunBridgeGetContractInfo_input
{
    uint8_t dummy;
};

struct OrderInfo
{
    uint8_t qubicSender[32];
    uint8_t qubicDestination[32];
    uint64_t orderId;
    uint64_t amount;
    uint8_t orderType;
    uint8_t status;
    uint8_t fromQubicToEthereum;
    uint8_t tokensReceived;
    uint8_t tokensLocked;
};

struct vottunBridgeGetContractInfo_output
{
    uint8_t admin[32];
    uint8_t managers[16][32];
    uint64_t nextOrderId;
    uint64_t lockedTokens;
    uint64_t totalReceivedTokens;
    uint64_t earnedFees;
    uint32_t tradeFeeBillionths;
    uint32_t sourceChain;
    OrderInfo firstOrders[16];
    uint64_t totalOrdersFound;
    uint64_t emptySlots;
    uint8_t numberOfAdmins;
    uint8_t requiredApprovals;
    uint64_t totalProposals;
    uint8_t multisigAdmins[16][32];

    static constexpr unsigned char type()
    {
        return RespondContractFunction::type();
    }
};

struct vottunBridgeGetAvailableFees_input
{
    uint8_t dummy;
};

struct vottunBridgeGetAvailableFees_output
{
    uint64_t availableFees;
    uint64_t totalEarnedFees;
    uint64_t totalDistributedFees;

    static constexpr unsigned char type()
    {
        return RespondContractFunction::type();
    }
};

// Query function declarations
void getOrder(const char* nodeIp, int nodePort, uint64_t orderId);
void getTotalReceivedTokens(const char* nodeIp, int nodePort, uint64_t amount);
void getAdminID(const char* nodeIp, int nodePort, uint8_t idInput);
void getTotalLockedTokens(const char* nodeIp, int nodePort);
void getOrderByDetails(const char* nodeIp, int nodePort, const char* ethAddress, uint64_t amount, uint8_t status);
void getContractInfo(const char* nodeIp, int nodePort);
void getAvailableFees(const char* nodeIp, int nodePort);
void getProposal(const char* nodeIp, int nodePort, uint64_t proposalId);
void addLiquidity(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, uint64_t amount);
void withdrawFees(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, uint64_t amount);