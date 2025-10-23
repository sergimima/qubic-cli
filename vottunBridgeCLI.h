#pragma once

#include <cstdint>

// VottunBridge Smart Contract CLI Functions

// Procedures (modify state)
void createOrder(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset,
                 const char* qubicDestination, const char* ethAddress, uint64_t amount, bool fromQubicToEthereum);

void setAdmin(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, const char* identity);

void addManager(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, const char* identity);

void removeManager(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, const char* identity);

void completeOrder(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, uint64_t orderId);

void refundOrder(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, uint64_t orderId);

void transferToContract(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, uint64_t amount, uint64_t orderId);

void addLiquidity(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, uint64_t amount);

void withdrawFees(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, uint64_t amount);

// Multisig Procedures
void createProposal(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset,
                    uint8_t proposalType, const char* targetAddress, uint64_t amount);

void approveProposal(const char* nodeIp, int nodePort, const char* seed, uint32_t scheduledTickOffset, uint64_t proposalId);

// Query Functions (read-only)
void getOrder(const char* nodeIp, int nodePort, uint64_t orderId);

void getTotalReceivedTokens(const char* nodeIp, int nodePort, uint64_t amount);

void getAdminID(const char* nodeIp, int nodePort, uint8_t idInput);

void getTotalLockedTokens(const char* nodeIp, int nodePort);

void getOrderByDetails(const char* nodeIp, int nodePort, const char* ethAddress, uint64_t amount, uint8_t status);

void getContractInfo(const char* nodeIp, int nodePort);

void getAvailableFees(const char* nodeIp, int nodePort);

// Multisig Query Functions
void getProposal(const char* nodeIp, int nodePort, uint64_t proposalId);
