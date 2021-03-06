/*
 * l3_v6_ipsec.c
 *
 *
 * Copyright (c) 2017-2020, AT&T Intellectual Property. All rights reserved.
 * Copyright (c) 2016, 2017 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */
#include <netinet/in.h>
#include <rte_branch_prediction.h>
#include <rte_ether.h>
#include <stdio.h>

#include "compiler.h"
#include "crypto/crypto_forward.h"
#include "netinet6/ip6_funcs.h"
#include "nh_common.h"
#include "pl_common.h"
#include "pl_fused.h"

ALWAYS_INLINE unsigned int
ipv6_ipsec_out_process(struct pl_packet *pkt, void *context __unused)
{
	struct ifnet *ifp = pkt->in_ifp;
	struct rte_mbuf *m = pkt->mbuf;
	struct next_hop *nh = NULL;

	/* Returns true if packet was consumed by IPsec */
	if (unlikely(crypto_policy_check_outbound(ifp, &m, pkt->tblid,
						  htons(RTE_ETHER_TYPE_IPV6),
						  &nh)))
		return IPV6_IPSEC_CONSUME;

	/*
	 * If the crypto code returned a next hop, then the policy
	 * matched but we need to run features attached to the interface
	 * the next hop is pointing at. The packet will then be put back
	 * in the crypto path.
	 */
	if (unlikely(nh != NULL))
		pkt->nxt.v6 = nh;

	if (unlikely(m != pkt->mbuf)) {
		pkt->mbuf = m;
		pkt->l3_hdr = ip6hdr(m);
	}
	return IPV6_IPSEC_ACCEPT;
}

/* Crypto encryption feature */
PL_REGISTER_NODE(ipv6_ipsec_out_node) = {
	.name = "vyatta:ipv6-ipsec-out",
	.type = PL_PROC,
	.handler = ipv6_ipsec_out_process,
	.num_next = IPV6_IPSEC_NUM,
	.next = {
		[IPV6_IPSEC_ACCEPT]   = "term-noop",
		[IPV6_IPSEC_CONSUME]   = "term-finish",
	}
};

PL_REGISTER_FEATURE(ipv6_ipsec_out_feat) = {
	.name = "vyatta:ipv6-ipsec-out",
	.node_name = "ipv6-ipsec-out",
	.feature_point = "ipv6-route-lookup",
	.id = PL_L3_V6_ROUTE_LOOKUP_FUSED_FEAT_IPSEC,
};
