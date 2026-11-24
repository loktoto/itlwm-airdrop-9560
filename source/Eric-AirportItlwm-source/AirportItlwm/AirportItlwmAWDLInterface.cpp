//
//  AirportItlwmAWDLInterface.cpp
//  AirportItlwm
//
//  awdl0 for macOS 26 (Tahoe) and macOS 15 (Sequoia), stage 1: accept and log the AWDL control plane.
//

#include "AirportItlwmV2.hpp"
#include "AirportItlwmAWDLInterface.hpp"
#include "AirportItlwmSkywalkInterface.hpp"
#include <HAL/ItlAWDLBootArg.hpp>

#ifdef AIRPORT_WCL

extern IOCommandGate *_fCommandGate;

OSDefineMetaClassAndStructors(AirportItlwmAWDLInterface, IO80211AWDLProtocol);
OSDefineMetaClassAndStructors(AirportItlwmAWDLEthernetInterface, AirportItlwmEthernetInterface);
OSDefineMetaClassAndStructors(AirportItlwmLowLatencyEthernetInterface, AirportItlwmEthernetInterface);

extern "C" {
unsigned int awdlGetInterfaceId(IO80211SkywalkInterface *)
    __asm__("__ZN23IO80211SkywalkInterface14getInterfaceIdEv");
void awdlSetLowLatencyEnabled(IO80211SkywalkInterface *, bool)
    __asm__("__ZN23IO80211SkywalkInterface20setLowLatencyEnabledEb");
void awdlSetCompanionInterfaceId(IO80211SkywalkInterface *, unsigned int)
    __asm__("__ZN23IO80211SkywalkInterface23setCompanionInterfaceIdEj");
void awdlSetParentInterface(IO80211SkywalkInterface *, IO80211VirtualInterface *)
    __asm__("__ZN23IO80211SkywalkInterface18setParentInterfaceEP23IO80211VirtualInterface");
void awdlSetCompanionLowLatencySkywalkInterface(IO80211VirtualInterface *, IO80211SkywalkInterface *)
    __asm__("__ZN23IO80211VirtualInterface38setCompanionLowLatencySkywalkInterfaceEP23IO80211SkywalkInterface");
}

#pragma mark - native table check

/*
 * This class overrides Apple's IO80211AWDLProtocol slot for slot. On a macOS build whose table moved,
 * IO80211 would call the wrong function in this object and panic, so awdl0 is only created when the
 * running table still has known functions at anchor slots spread over its whole length, its own pure
 * methods where expected, and ends where expected. Anchors as in ForbiddenItlwm's AWDLBuildGuard
 * (retropineaple/ForbiddenItlwm, GPL-2.0), checked against the AWDL-ABI reports of 25G229, 25G332 and
 * 25G342 (identical 568-slot tables) and of 24H232 (563 slots). Slots count from the address point.
 */
extern "C" {
extern const uintptr_t awdlNativeProtocolVtable[] __asm__("__ZTV19IO80211AWDLProtocol");
void awdlAnchorProtocolD1() __asm__("__ZN19IO80211AWDLProtocolD1Ev");
void awdlAnchorWakeFlags() __asm__("__ZN25IOSkywalkNetworkInterface21getSupportedWakeFlagsEPj");
void awdlAnchorInterfaceCCA() __asm__("__ZN23IO80211SkywalkInterface15setInterfaceCCAE18apple80211_channeli");
void awdlAnchorStatsDump() __asm__("__ZN23IO80211SkywalkInterface9statsDumpEv");
#if __IO80211_TARGET == __MAC_15_2
void awdlAnchorVirtualInit() __asm__("__ZN23IO80211VirtualInterface4initEP17IO80211ControllerP10ether_addrjPKc");
#else
void awdlAnchorSyncDPSStats() __asm__("__ZN23IO80211SkywalkInterface12syncDPSStatsEP26apple82011_postMessage_dps");
#endif
void awdlAnchorReserved15() __asm__("__ZN23IO80211VirtualInterface34_RESERVEDIO80211VirtualInterface15Ev");
void awdlAnchorPureVirtual() __asm__("___cxa_pure_virtual");
}

const char *AirportItlwmAWDLInterface::
nativeTableMismatch()
{
    struct Anchor { unsigned slot; uintptr_t expected; };
    const uintptr_t pure = reinterpret_cast<uintptr_t>(&awdlAnchorPureVirtual);
    const Anchor anchors[] = {
        {0, reinterpret_cast<uintptr_t>(&awdlAnchorProtocolD1)},
#if __IO80211_TARGET == __MAC_15_2
        {299, reinterpret_cast<uintptr_t>(&awdlAnchorWakeFlags)},
        {380, reinterpret_cast<uintptr_t>(&awdlAnchorInterfaceCCA)},
        {415, pure},        // setMacAddress, pure in IO80211SkywalkInterface on macOS 15
        {420, reinterpret_cast<uintptr_t>(&awdlAnchorStatsDump)},
        {460, reinterpret_cast<uintptr_t>(&awdlAnchorVirtualInit)},
        {499, reinterpret_cast<uintptr_t>(&awdlAnchorReserved15)},
        {500, pure},        // first IO80211AWDLProtocol method
        {562, pure},        // last one
        {563, 0},
#elif defined(ITL_MACOS27)
        // 27.2.1 (26B131), 579 slots: IOSkywalkNetworkInterface gained five IPsec virtuals at slot 323 and
        // IO80211SkywalkInterface / IO80211VirtualInterface three and two more of their own.
        {300, reinterpret_cast<uintptr_t>(&awdlAnchorWakeFlags)},
        {386, reinterpret_cast<uintptr_t>(&awdlAnchorInterfaceCCA)},
        {426, reinterpret_cast<uintptr_t>(&awdlAnchorStatsDump)},
        {468, reinterpret_cast<uintptr_t>(&awdlAnchorSyncDPSStats)},
        {514, reinterpret_cast<uintptr_t>(&awdlAnchorReserved15)},
        {515, pure},        // first IO80211AWDLProtocol method
        {578, pure},        // last one
        {579, 0},
#else
        {300, reinterpret_cast<uintptr_t>(&awdlAnchorWakeFlags)},
        {380, reinterpret_cast<uintptr_t>(&awdlAnchorInterfaceCCA)},
        {420, reinterpret_cast<uintptr_t>(&awdlAnchorStatsDump)},
        {460, reinterpret_cast<uintptr_t>(&awdlAnchorSyncDPSStats)},
        {504, reinterpret_cast<uintptr_t>(&awdlAnchorReserved15)},
        {505, pure},        // first IO80211AWDLProtocol method
        {567, pure},        // last one
        {568, 0},
#endif
    };
    const uintptr_t *slots = awdlNativeProtocolVtable + 2;
    for (const Anchor &a : anchors)
        if (slots[a.slot] != a.expected)
            return a.expected ? "blocked: IO80211AWDLProtocol table differs" :
                "blocked: IO80211AWDLProtocol table is longer than expected";
    return nullptr;
}

// The framework frees mExpansionData[2]->fRegistrationInfo with kfree_type when an interface is stopped (Tahoe
// IOSkywalkFamily: deregisterNetworkInterface and deregisterEthernetInterface null-check the pointer, then free it as a typed
// 288-byte / Ethernet object). Our own IOMalloc'd records are in other zones. In a normal run registerEthernetInterface
// replaces them with typed copies; when it was never reached, stop() would free ours. So: if the pointer is still ours, take
// it out before stop(), and release our buffers ourselves afterwards.
static void takeOutOwnRegistrationInfo(IOSkywalkNetworkInterface *netIf, IOSkywalkEthernetInterface *ethIf, void *ownNet, void *ownEth)
{
    if (netIf && netIf->mExpansionData && ownNet && netIf->mExpansionData->fRegistrationInfo == ownNet)
        netIf->mExpansionData->fRegistrationInfo = nullptr;
    if (ethIf && ethIf->mExpansionData2 && ownEth && ethIf->mExpansionData2->fRegistrationInfo == ownEth)
        ethIf->mExpansionData2->fRegistrationInfo = nullptr;
}

static void freeOwnRegistrationInfo(void *&ownNet, void *&ownEth)
{
    if (ownNet) IOFree(ownNet, sizeof(IOSkywalkNetworkInterface::RegistrationInfo));
    if (ownEth) IOFree(ownEth, sizeof(IOSkywalkEthernetInterface::RegistrationInfo));
    ownNet = ownEth = nullptr;
}

#define AWDL_LOG(fmt, ...) XYLogE("AWDL %s: " fmt "\n", __FUNCTION__, ##__VA_ARGS__)

#pragma mark - awdl0 BSD interface

const char *AirportItlwmAWDLEthernetInterface::
getNamePrefix() const
{
    return "awdl";
}

void AirportItlwmLowLatencyEthernetInterface::
setLowLatencyAddress(const ether_addr &mac)
{
    memcpy(&fMac, &mac, sizeof(fMac));
    memcpy(fUniqueId, mac.octet, sizeof(mac.octet));
    fUniqueId[6] = kAirportItlwmLowLatencyInterfaceId;
}

bool AirportItlwmLowLatencyEthernetInterface::
initIfnetParams(struct ifnet_init_params *params)
{
    setProperty(kIOMACAddress, (void *)&fMac, kIOEthernetAddressSize);
    setProperty("HiddenInterface", kOSBooleanTrue);
    if (!AirportItlwmEthernetInterface::initIfnetParams(params))
        return false;
    params->uniqueid = fUniqueId;
    params->uniqueid_len = sizeof(fUniqueId);
    return true;
}

IOReturn AirportItlwmLowLatencyEthernetInterface::
attachToDataLinkLayer(IOOptionBits options, void *parameter)
{
    IOReturn ret = AirportItlwmEthernetInterface::attachToDataLinkLayer(options, parameter);
    if (ret == kIOReturnSuccess && getIfnet())
        ifnet_set_lladdr(getIfnet(), &fMac, sizeof(fMac));
    fBSDRegistrationResult = ret;
    __atomic_store_n(&fBSDRegistrationComplete, true, __ATOMIC_RELEASE);
    if (_fCommandGate)
        _fCommandGate->commandWakeup(this);
    return ret;
}

IOReturn AirportItlwmLowLatencyEthernetInterface::
getInterfaceAddress(IOEthernetAddress *addr)
{
    memcpy(addr, &fMac, sizeof(fMac));
    return kIOReturnSuccess;
}

bool AirportItlwmLowLatencyEthernetInterface::
setLinkState(IO80211LinkState state)
{
    if (!getIfnet())
        return false;
    ifnet_set_flags(getIfnet(), state == kIO80211NetworkLinkUp ? IFF_RUNNING : 0,
                    state == kIO80211NetworkLinkUp ? 0 : IFF_RUNNING);
    return true;
}

SInt32 AirportItlwmLowLatencyEthernetInterface::
performCommand(IONetworkController *controller, unsigned long cmd, void *arg0, void *arg1)
{
    if (cmd == SIOCSIFFLAGS) {
        auto *driver = OSDynamicCast(AirportItlwm, getController());
        auto *llw = OSDynamicCast(AirportItlwmLowLatencyInterface, getProvider());
        if (driver && driver->fHalService && llw) {
            ifnet_t ifp = getIfnet();
            bool up = ifp && (ifnet_flags(ifp) & IFF_UP) &&
                driver->fHalService->get80211Controller()->ic_state == IEEE80211_S_RUN;
            llw->setRunningState(up);
            llw->reportLinkStatus(up ? 3 : 1, up ? 0x80 : 0);
        }
        return 0;
    }
    return AirportItlwmEthernetInterface::performCommand(controller, cmd, arg0, arg1);
}

UInt32 AirportItlwmLowLatencyEthernetInterface::
outputLowLatency(mbuf_t packet, void *)
{
    AirportItlwm *controller = OSDynamicCast(AirportItlwm, getController());
    if (!controller) {
        if (packet)
            mbuf_freem(packet);
        return kIOReturnOutputDropped;
    }
    auto *ic = controller->fHalService->get80211Controller();
    if (!ic || !ic->ic_awdl_llw_tx || !controller->fAWDLBsdIf) {
        if (packet)
            mbuf_freem(packet);
        return kIOReturnOutputDropped;
    }
    return controller->fAWDLBsdIf->outputLowLatency(packet, this);
}

bool AirportItlwmAWDLEthernetInterface::
initIfnetParams(struct ifnet_init_params *params)
{
    // Publish awdl0's own address before IOEthernetInterface reads it as the
    // ifnet uniqueid; en6 already owns the station MAC.
    setProperty(kIOMACAddress, (void *)&fMac, kIOEthernetAddressSize);
    // The controller is an IO80211Controller, so configd would list awdl0 as a
    // second Wi-Fi network service ("Wi-Fi 2") and run DHCP on it. A network
    // interface carrying HiddenInterface is left out of the service list.
    setProperty("HiddenInterface", kOSBooleanTrue);

    if (!AirportItlwmEthernetInterface::initIfnetParams(params))
        return false;
    params->uniqueid = &fMac;
    params->uniqueid_len = sizeof(fMac);
    return true;
}

IOReturn AirportItlwmAWDLEthernetInterface::
attachToDataLinkLayer(IOOptionBits options, void *parameter)
{
    IOReturn ret = AirportItlwmEthernetInterface::attachToDataLinkLayer(options, parameter);
    if (ret == kIOReturnSuccess && getIfnet()) {
        errno_t err = ifnet_set_lladdr(getIfnet(), &fMac, sizeof(fMac));
        AWDL_LOG("lladdr %s err %d", ether_sprintf(fMac.octet), err);
    }
    return ret;
}

SInt32 AirportItlwmAWDLEthernetInterface::
performCommand(IONetworkController *controller, unsigned long cmd, void *arg0, void *arg1)
{
    // The default SIOCSIFFLAGS path calls controller->enable(this), which would
    // reinitialise the shared radio. The flags are already set on the ifnet.
    if (cmd == SIOCSIFFLAGS) {
        AWDL_LOG("flags 0x%x", ifnet_flags(getIfnet()));
        // Link state is what starts IO80211AWDLPeerManager::configure(). It has
        // to run in the gate but off the work queue thread.
        uintptr_t up = (ifnet_flags(getIfnet()) & IFF_UP) != 0;
        if (_fCommandGate)
            _fCommandGate->runAction(AirportItlwm::setAWDLLinkGated, (void *)up);
        return 0;
    }
    return AirportItlwmEthernetInterface::performCommand(controller, cmd, arg0, arg1);
}

static bool awDataAllowed();
static uint64_t hostUs();

int AirportItlwmAWDLEthernetInterface::
inputAWDLData(mbuf_t m, AirportItlwmAWDLInterface *awdl, ieee80211com *ic)
{
    if (!awDataAllowed() || !getIfnet())
        return 0;
    if (mbuf_pullup(&m, mbuf_pkthdr_len(m) < 64 ? mbuf_pkthdr_len(m) : 64) != 0 || !m) {
        awdl->fDataRxBad++;
        return 1;   // mbuf_pullup freed it
    }
    if (ic && ic->ic_awdl_crypto &&
        (mtod(m, struct ieee80211_frame *)->i_fc[1] & IEEE80211_FC1_PROTECTED)) {
        int err = ic->ic_awdl_crypto(ic, &m, 0);
        if (err || !m) {
            awdl->fDataRxBad++;
            if (m)
                mbuf_freem(m);
            return 1;
        }
    }
    const uint8_t *p = (const uint8_t *)mbuf_data(m);
    size_t len = mbuf_len(m);
    const struct ieee80211_frame *wh = (const struct ieee80211_frame *)p;
    size_t hdr = sizeof(*wh);
    static const uint8_t kSnap[6] = { 0xaa, 0xaa, 0x03, 0x00, 0x17, 0xf2 };
    static const uint8_t kLlwSnap[6] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };
    auto *driver = OSDynamicCast(AirportItlwm, getController());
    auto *llw = driver ? driver->fLLWBsdIf : nullptr;
    ifnet_t llwIf = llw && llw->bsdRegistrationComplete() &&
        llw->bsdRegistrationResult() == kIOReturnSuccess ? llw->getIfnet() : nullptr;
    if (ieee80211_has_qos(wh)) {
        hdr = ieee80211_get_hdrlen(wh);
        if (len < hdr) {
            awdl->fDataRxBad++;
            mbuf_freem(m);
            return 1;
        }
        const uint16_t qos = ieee80211_get_qos(wh);
        if (qos & IEEE80211_QOS_AMSDU) {
            bool deliveredAWDL = false;
            size_t offset = hdr, total = mbuf_pkthdr_len(m);
            while (offset + 22 <= total) {
                uint8_t subhdr[30];
                if (mbuf_copydata(m, offset, 22, subhdr) != 0) {
                    awdl->fDataRxBad++;
                    break;
                }
                size_t sublen = (subhdr[12] << 8) | subhdr[13];
                if (sublen < 8 || offset + 14 + sublen > total) {
                    awdl->fDataRxBad++;
                    break;
                }
                const bool lowLatency = memcmp(subhdr + 14, kLlwSnap, sizeof(kLlwSnap)) == 0;
                const size_t payloadOffset = lowLatency ? 22 : 30;
                const size_t consumed = 14 + sublen;
                if (lowLatency ? !llwIf :
                    (sublen < 16 || memcmp(subhdr + 14, kSnap, sizeof(kSnap)) ||
                     subhdr[20] != 0x08 || subhdr[21] != 0x00 ||
                     mbuf_copydata(m, offset + 22, 8, subhdr + 22) != 0)) {
                    awdl->fDataRxBad++;
                    offset += (consumed + 3) & ~size_t(3);
                    continue;
                }
                ifnet_t inputIf = lowLatency ? llwIf : getIfnet();
                mbuf_t sub = nullptr;
                if (mbuf_gethdr(MBUF_DONTWAIT, MBUF_TYPE_DATA, &sub) || !sub) {
                    awdl->fDataRxNoBuf++;
                    break;
                }
                uint8_t eh[14];
                memcpy(eh, subhdr, 12);
                eh[12] = lowLatency ? subhdr[20] : subhdr[28];
                eh[13] = lowLatency ? subhdr[21] : subhdr[29];
                int err = mbuf_copyback(sub, 0, sizeof(eh), eh, MBUF_DONTWAIT);
                size_t payload = sublen - (lowLatency ? 8 : 16);
                uint8_t chunk[512];
                for (size_t pos = 0; !err && pos < payload;) {
                    size_t n = MIN(sizeof(chunk), payload - pos);
                    err = mbuf_copydata(m, offset + payloadOffset + pos, n, chunk);
                    if (!err)
                        err = mbuf_copyback(sub, sizeof(eh) + pos, n, chunk, MBUF_DONTWAIT);
                    pos += n;
                }
                if (err) {
                    mbuf_freem(sub);
                    awdl->fDataRxBad++;
                    break;
                }
                mbuf_pkthdr_setlen(sub, sizeof(eh) + payload);
                mbuf_pkthdr_setrcvif(sub, inputIf);
                if (!lowLatency) {
                    deliveredAWDL = true;
                    awdl->fDataRx++;
                    awdl->fDataRxLastLen = (uint32_t)mbuf_pkthdr_len(sub);
                    awdl->fDataRxLastType = (uint16_t)((eh[12] << 8) | eh[13]);
                }
                ifnet_stat_increment_in(inputIf, 1, (u_int32_t)mbuf_pkthdr_len(sub), 0);
                if (lowLatency)
                    llw->inputPacket(sub, (UInt32)mbuf_pkthdr_len(sub), 0, sub);
                else
                    inputPacket(sub, (UInt32)mbuf_pkthdr_len(sub), 0, sub);
                offset += (consumed + 3) & ~size_t(3);
            }
            mbuf_freem(m);
            if (deliveredAWDL) {
                awdl->fDataRxAmsdu++;
                awdl->publishDataState();
            }
            return 1;
        }
    }
    if (len >= hdr + 8 && !memcmp(p + hdr, kLlwSnap, sizeof(kLlwSnap))) {
        if (!llwIf) {
            awdl->fDataRxBad++;
            mbuf_freem(m);
            return 1;
        }
        uint8_t ethType[2] = { p[hdr + 6], p[hdr + 7] };
        uint8_t dst[6], src[6];
        memcpy(dst, wh->i_addr1, 6);
        memcpy(src, wh->i_addr2, 6);
        mbuf_adj(m, (int)(hdr + 8 - 14));
        uint8_t *eh = (uint8_t *)mbuf_data(m);
        memcpy(eh, dst, 6);
        memcpy(eh + 6, src, 6);
        eh[12] = ethType[0];
        eh[13] = ethType[1];
        mbuf_pkthdr_setrcvif(m, llwIf);
        ifnet_stat_increment_in(llwIf, 1, (u_int32_t)mbuf_pkthdr_len(m), 0);
        llw->inputPacket(m, (UInt32)mbuf_pkthdr_len(m), 0, m);
        return 1;
    }
    const size_t llc = 8, awdlHdr = 8;
    if (len < hdr + llc + awdlHdr || memcmp(p + hdr, kSnap, sizeof(kSnap)) ||
        p[hdr + 6] != 0x08 || p[hdr + 7] != 0x00) {
        awdl->fDataRxBad++;
        mbuf_freem(m);
        return 1;
    }
    const uint8_t *ad = p + hdr + llc;
    uint8_t ethType[2] = { ad[6], ad[7] };
    uint8_t dst[6], src[6];
    memcpy(dst, wh->i_addr1, 6);
    memcpy(src, wh->i_addr2, 6);

    // Keep 14 bytes of the stripped headers for the Ethernet header.
    size_t strip = hdr + llc + awdlHdr - 14;
    mbuf_adj(m, (int)strip);
    uint8_t *eh = (uint8_t *)mbuf_data(m);
    memcpy(eh, dst, 6);
    memcpy(eh + 6, src, 6);
    eh[12] = ethType[0];
    eh[13] = ethType[1];
    mbuf_pkthdr_setrcvif(m, getIfnet());
    awdl->fDataRx++;
    awdl->fDataRxLastLen = (uint32_t)mbuf_pkthdr_len(m);
    awdl->fDataRxLastType = (uint16_t)((ethType[0] << 8) | ethType[1]);
    if (awdl->fDataRx <= 4 || (awdl->fDataRx & 0x1f) == 0)
        awdl->publishDataState();
    // IONetworkInterface does not account these on awdl0 (netstat showed 0 in,
    // bytes included), so count them on the ifnet here.
    ifnet_stat_increment_in(getIfnet(), 1, (u_int32_t)mbuf_pkthdr_len(m), 0);
    inputPacket(m, (UInt32)mbuf_pkthdr_len(m), 0, m);
    return 1;
}

// Runs on the network output thread: move the frame onto the work loop.
UInt32 AirportItlwmAWDLEthernetInterface::
outputAWDL(mbuf_t m, void *param)
{
    if (!awDataAllowed() || !fOwner || !_fCommandGate)
        return dropOutput(m, param);
    _fCommandGate->runAction([](OSObject *, void *owner, void *pkt, void *, void *) -> IOReturn {
        static_cast<AirportItlwmAWDLInterface *>(owner)->enqueueData((mbuf_t)pkt);
        return kIOReturnSuccess;
    }, fOwner, m);
    return kIOReturnOutputSuccess;
}

UInt32 AirportItlwmAWDLEthernetInterface::
outputLowLatency(mbuf_t m, AirportItlwmLowLatencyEthernetInterface *source)
{
    if (!awDataAllowed() || !fOwner || !_fCommandGate)
        return dropOutput(m, nullptr);
    IOReturn result = _fCommandGate->runAction([](OSObject *target, void *owner, void *pkt, void *sender, void *) -> IOReturn {
        auto *awdl = static_cast<AirportItlwmAWDLInterface *>(owner);
        auto *driver = static_cast<AirportItlwm *>(target);
        auto *llw = driver->fLLWBsdIf;
        if (!llw || llw != sender || !llw->bsdRegistrationComplete() ||
            llw->bsdRegistrationResult() != kIOReturnSuccess)
            return kIOReturnNotReady;
        awdl->enqueueLowLatencyData((mbuf_t)pkt);
        awdl->flushLowLatencyData();
        return kIOReturnSuccess;
    }, fOwner, m, source);
    return result == kIOReturnSuccess ? kIOReturnOutputSuccess : dropOutput(m, nullptr);
}

UInt32 AirportItlwmAWDLEthernetInterface::
dropOutput(mbuf_t m, void *param)
{
    fDropped++;
    mbuf_freem(m);
    return kIOReturnOutputDropped;
}

// Stage 2c is opt-in while it is being brought up: a test machine boot-looped
// with it always on. Without -itlawdlaw awdl0 only receives action frames.
// Stage 2b, also opt-in: visit the social channel when the station is not on it.
static bool awListenAllowed()
{
    static int allowed = -1;
    if (allowed < 0) {
        int v;
        allowed = itlAWDLBootFlag("-itlawdllisten") ? 1 : 0;
    }
    return allowed == 1;
}

// Diagnostic: itlawdllistench=<n> sends every listen dwell to channel n
// instead of the AW slot channel, e.g. the channel the phone sits on.
static unsigned awListenChannelOverride()
{
    static int chan = -1;
    if (chan < 0) {
        int v = 0;
        chan = PE_parse_boot_argn("itlawdllistench", &v, sizeof(v)) && v > 0 && v <= 196 ? v : 0;
    }
    return (unsigned)chan;
}

// -itlawdlhunt (diagnostic): while no peer has been heard, spend most of the time on the
// social channels in turn instead of the sparse slots of our own sequence.
static bool awHuntAllowed()
{
    static int allowed = -1;
    if (allowed < 0) {
        int v = 0;
        allowed = itlAWDLBootFlag("-itlawdlhunt") ? 1 : 0;
    }
    return allowed == 1;
}

// -itlawdlnopeerlisten turns off listening on the tracked peer's channel in
// slots where our own sequence stays home.
static bool awPeerListenAllowed()
{
    static int allowed = -1;
    if (allowed < 0) {
        int v = 0;
        allowed = PE_parse_boot_argn("-itlawdlnopeerlisten", &v, sizeof(v)) ? 0 : 1;
    }
    return allowed == 1;
}

// Stage 4: AWDL data frames to and from awdl0's BSD interface.
static bool awDataAllowed()
{
    static int allowed = -1;
    if (allowed < 0) {
        int v;
        allowed = itlAWDLBootFlag("-itlawdldata") ? 1 : 0;
    }
    return allowed == 1;
}

// Stage 3: transmit AWDL frames during the listen dwells.
static bool awTxAllowed()
{
    static int allowed = -1;
    if (allowed < 0) {
        int v;
        allowed = itlAWDLBootFlag("-itlawdltx") ? 1 : 0;
    }
    return allowed == 1;
}

// Report the transmit status of a unicast action frame (see the member comment). Part of -itlawdl; the single flag
// -itlawdlumiack still works on its own.
static bool awUmiAckAllowed()
{
    static int allowed = -1;
    if (allowed < 0)
        allowed = itlAWDLBootFlag("-itlawdlumiack") ? 1 : 0;
    return allowed == 1;
}

static bool awTimelineAllowed()
{
    static int allowed = -1;
    if (allowed < 0) {
        int v;
        allowed = itlAWDLBootFlag("-itlawdlaw") ? 1 : 0;
    }
    return allowed == 1;
}

#pragma mark - receive path, mirrors AppleBCMWLANProximityInterface::handleActionFrame_rx

#if __IO80211_TARGET == __MAC_15_2
// Sequoia: IO80211Buffer has no virtual methods (plain exports), the pool is the controller's, and
// bpfTapInput takes over the buffer.
typedef IO80211BufferPool AWDLBufferPoolABI;
typedef IO80211Buffer AWDLBufferABI;
static inline AWDLBufferPoolABI *awdlMgmtPool(IO80211VirtualInterface *, IO80211Controller *c) { return itlGetMgmtFramePool(c); }
static inline AWDLBufferABI *awdlBufGet(AWDLBufferPoolABI *pool) { return pool->getBuffer(); }
static inline size_t awdlBufCapacity(AWDLBufferABI *b) { return b->getCapacity(); }
static inline uint8_t *awdlBufBytes(AWDLBufferABI *b) { return (uint8_t *)b->getBytesNoCopy(); }
static inline void awdlBufSetLength(AWDLBufferABI *b, size_t n) { b->setLength(n); }
static inline void awdlBufReturn(AWDLBufferABI *b) { b->returnBuffer(); }
static inline void awdlBufSubmit(IO80211VirtualInterface *i, AWDLBufferABI *b, void *meta, size_t n)
{
    itlSubmitMgmtFrame(i, b, 12, meta, n);
}
#else
// IO80211BufferPool / IO80211Buffer vtable slots (Tahoe KC, address point relative).
enum : size_t {
    kPoolGetBuffer   = 0x118,
    kBufReturn       = 0x118,
    kBufSetLength    = 0x138,
    kBufGetCapacity  = 0x140,
    kBufGetBytes     = 0x158,
};
template <typename R, typename... A>
static inline R vcall(const void *obj, size_t slot, A... args)
{
    auto fn = *reinterpret_cast<R (* const *)(const void *, A...)>(*reinterpret_cast<const uint8_t * const *>(obj) + slot);
    return fn(obj, args...);
}
static inline AWDLBufferPoolABI *awdlMgmtPool(IO80211VirtualInterface *i, IO80211Controller *)
{
    return IO80211VirtualInterface_getMgmtFramePool(i);
}
static inline AWDLBufferABI *awdlBufGet(AWDLBufferPoolABI *pool) { return vcall<AWDLBufferABI *>(pool, kPoolGetBuffer); }
static inline size_t awdlBufCapacity(AWDLBufferABI *b) { return vcall<size_t>(b, kBufGetCapacity); }
static inline uint8_t *awdlBufBytes(AWDLBufferABI *b) { return vcall<uint8_t *>(b, kBufGetBytes); }
static inline void awdlBufSetLength(AWDLBufferABI *b, size_t n) { vcall<void>(b, kBufSetLength, n); }
static inline void awdlBufReturn(AWDLBufferABI *b) { vcall<void>(b, kBufReturn); }
static inline void awdlBufSubmit(IO80211VirtualInterface *i, AWDLBufferABI *b, void *meta, size_t n)
{
    IO80211VirtualInterface_bpfTapInput(i, b, 12, meta, n);
}
#endif

// Metadata AppleBCMWLANProximityInterface passes to bpfTapInput (0x48 bytes).
struct AWDLRxMeta {
    uint8_t  addr[6];          // 0x00 transmitter
    uint8_t  pad0[6];
    apple80211_channel chan;   // 0x0c
    uint8_t  pad1[0x28 - 0x18];
    uint32_t rate;             // 0x28
    int32_t  rssi;             // 0x2c, peer manager drops new peers below a threshold
    uint32_t tsf;              // 0x30 receive timestamp (us)
    uint8_t  pad2[0x48 - 0x34];
};
static_assert(sizeof(AWDLRxMeta) == 0x48, "AWDL rx meta size");
static_assert(offsetof(AWDLRxMeta, chan) == 0x0c, "AWDL rx meta chan");
static_assert(offsetof(AWDLRxMeta, rssi) == 0x2c, "AWDL rx meta rssi");

// Category 127, OUI 00:17:f2, type 8 (AWDL), version 0: checked by
// IO80211AWDLPeerManager::actionFrameHasCorrectOUI over the first 16 bytes.
static const uint8_t kAWDLFixedHeaderLen = 16;

void AirportItlwmAWDLInterface::
noteRxGap(const uint8_t *src, uint64_t nowUs)
{
    RxGap *e = nullptr, *oldest = &fRxGap[0];
    for (unsigned i = 0; i < kRxGapSlots; i++) {
        if (!memcmp(fRxGap[i].mac, src, 6) && fRxGap[i].lastUs) {
            e = &fRxGap[i];
            break;
        }
        if (fRxGap[i].lastUs < oldest->lastUs)
            oldest = &fRxGap[i];
    }
    if (e) {
        uint64_t gap = nowUs - e->lastUs;
        if (gap > e->maxGapUs)
            e->maxGapUs = gap;
        if (gap > 6000000)
            e->over6s++;
    } else {
        e = oldest;
        memset(e, 0, sizeof(*e));
        memcpy(e->mac, src, 6);
    }
    e->lastUs = nowUs;
    e->count++;
}

int AirportItlwmAWDLInterface::
inputActionFrame(const uint8_t *frame, size_t len, const ieee80211_rxinfo *rxi)
{
    const size_t hdr = sizeof(struct ieee80211_frame);
    if (len < hdr + kAWDLFixedHeaderLen)
        return 0;
    // The firmware path delivers the frame body, starting at the category byte.
    const uint8_t *af = frame + hdr;
    if (af[0] != 127 || af[1] != 0x00 || af[2] != 0x17 || af[3] != 0xf2 || af[4] != 0x08)
        return 0;

    fRxAF++;
    noteRxGap(((const struct ieee80211_frame *)frame)->i_addr2, hostUs());
    uint64_t *chanCount = rxi->rxi_chan == 6 ? &fRxAFChan6 :
        rxi->rxi_chan == 44 ? &fRxAFChan44 :
        rxi->rxi_chan == 149 ? &fRxAFChan149 : &fRxAFChanOther;
    if (rxi->rxi_chan == 161)
        fRxAFChan161++;
    bool firstOnChan = (*chanCount)++ == 0;
    // A frame on a social channel is rare and is the result 2b is looking for.
    bool socialChan = chanCount != &fRxAFChanOther;
    // XYLog goes to kprintf, so publish progress where ioreg can see it.
    if (firstOnChan || socialChan || fRxAF <= 8 || (fRxAF & 0xf) == 0) {
        publishListenState();
        setProperty("AWDLRxFrames", fRxAF, 64);
        setProperty("AWDLRxDropped", fRxAFDropped, 64);
        setProperty("AWDLRxLastChannel", rxi->rxi_chan, 32);
    }

    AWDLBufferPoolABI *pool = awdlMgmtPool(this, fController);
    AWDLBufferABI *buf = pool ? awdlBufGet(pool) : nullptr;
    if (!buf) {
        fRxAFDropped++;
        return 1;
    }
    size_t payload = len - hdr;
    if (payload > awdlBufCapacity(buf)) {
        awdlBufReturn(buf);
        fRxAFDropped++;
        return 1;
    }
    // Copied verbatim: bytes 8..15 are phy_tx_time / target_tx_time, which the
    // peer manager uses for time sync.
    memcpy(awdlBufBytes(buf), af, payload);
    awdlBufSetLength(buf, payload);

    const struct ieee80211_frame *wh = (const struct ieee80211_frame *)frame;
    AWDLRxMeta meta = {};
    memcpy(meta.addr, wh->i_addr2, sizeof(meta.addr));
    meta.chan.version = APPLE80211_VERSION;
    meta.chan.channel = rxi->rxi_chan;
    meta.chan.flags = APPLE80211_C_FLAG_20MHZ |
        (rxi->rxi_chan <= 14 ? APPLE80211_C_FLAG_2GHZ : APPLE80211_C_FLAG_5GHZ);
    // rxi_rssi is normalized to 0..67 (dBm - IWX_MIN_DBM); the peer manager
    // compares dBm against its threshold.
    meta.rssi = rxi->rxi_rssi - 100;
    // Same clock as the AW start times we report: the peer manager subtracts
    // one from the other (IO80211AWDLPeer::calculatePeerTimeDiff).
    uint64_t rxUs = awTimelineAllowed() ? hostRxUs(rxi) : 0;
    meta.tsf = awTimelineAllowed() ? (uint32_t)rxUs : rxi->rxi_tstamp;

    if (fRxAF <= 8 || (fRxAF & 0xff) == 0) {
        // Fixed header bytes 5..7 (version, subtype, reserved) of real frames.
        char hv[8];
        snprintf(hv, sizeof(hv), "%02x%02x%02x", af[5], af[6], af[7]);
        setProperty("AWDLRxHdrVer", hv);
    }
    trackPeer(af, payload, wh->i_addr2, rxUs, rxi->rxi_chan);
    syncFromFrame(af, payload, wh->i_addr2, rxUs);

    // Consumes buf (peer manager input + bpf, then returnBuffer).
    awdlBufSubmit(this, buf, &meta, sizeof(meta));
    return 1;
}

#pragma mark - availability window timeline (host side of the firmware AW engine)

// Layout as read by AppleBCMWLANProximityInterface::buildAWDLChanSequence and
// IO80211AWDLPeerManager::setAwdlChannelSequence (the header's struct is stale):
// +5 entry count, +6 encoding, +7 step count (presence mode - 1),
// +12 entries of apple80211_channel (12 bytes each). 400 bytes in total.
enum : size_t {
    kChanSeqCount = 5,
    kChanSeqStep = 7,
    kChanSeqEntries = 12,
    kChanSeqMax = 32,
};

// Payloads of the AW messages, as built by AppleBCMWLANProximityInterface::
// handleAvailabilityWindowStart/End and read by IO80211AWDLPeerManager::
// reportDataPathEvents / monitorTimeline / logAWStart / logAWEnd. Times are
// big-endian microseconds; the peer manager subtracts the AW start time from
// the rx timestamp in the action frame meta, so both use hostUs().
// Posted through IO80211VirtualInterface::postMessage, which hands them to
// reportDataPathEvents -> IO80211AWDLPeerManager::reportDataPathEvents.
enum : unsigned int {
    kAWDLMsgAWStart = 41,
    kAWDLMsgAWExtension = 42,
    kAWDLMsgAWEnd = 43,
};

struct AWDLAWStartEvent {
    uint32_t seq;              // 0x00 AW sequence number (AWSeqNum)
    uint32_t role;             // 0x04 1 master, 2 slave, 3 non-election master
    uint8_t  master[6];        // 0x08 master we are synced to
    uint8_t  topMaster[6];     // 0x0e root of the sync tree
    uint32_t startUsBE;        // 0x14 AW start time
    apple80211_channel chan;   // 0x18 channel of this slot
} __attribute__((packed));
static_assert(sizeof(AWDLAWStartEvent) == 0x24, "AW start event size");

struct AWDLAWEndEvent {
    uint32_t extCount;         // 0x00
    uint32_t endUsBE;          // 0x04 AW end time
    uint8_t  pad[0x28 - 8];    // 0x08 extension and CCA statistics
} __attribute__((packed));
static_assert(sizeof(AWDLAWEndEvent) == 0x28, "AW end event size");

static const uint8_t kAWDLSyncParamsTLV = 4;
static const uint8_t kAWDLChanSeqTLV = 18;
static const int64_t kTimerEarlyUs = 2000;     // timer may fire this much early

static uint64_t hostUs()
{
    uint64_t ns;
    absolutetime_to_nanoseconds(mach_absolute_time(), &ns);
    return ns / 1000;
}

uint64_t AirportItlwmAWDLInterface::
hostRxUs(const ieee80211_rxinfo *rxi)
{
    uint64_t now = hostUs();
    if (!rxi->rxi_tstamp)
        return now;
    // The NIC TSF and the host clock drift slowly and the TSF can jump when it
    // follows the AP. Track the smallest host-minus-TSF offset (least rx
    // latency), let it creep up slowly for drift and reset on large jumps.
    uint32_t off = (uint32_t)now - rxi->rxi_tstamp;
    int32_t diff = (int32_t)(off - fTsfOffset);
    if (!fTsfValid || diff < 0 || diff > 20000) {
        fTsfOffset = off;
        fTsfValid = true;
    } else {
        fTsfOffset += diff < 20 ? diff : 20;
    }
    uint32_t latency = off - fTsfOffset;
    return latency < 1000000 ? now - latency : now;
}

uint16_t AirportItlwmAWDLInterface::
presenceMode() const
{
    // The peer manager steps AWSeqNum by its presence mode per channel slot and
    // tells us that through the channel sequence step count.
    if (fChanSeqValid && fChanSeq[kChanSeqStep] < 16)
        return fChanSeq[kChanSeqStep] + 1;
    if (fPresenceMode >= 1 && fPresenceMode <= 16)
        return fPresenceMode;
    return 4;
}

uint64_t AirportItlwmAWDLInterface::
slotUs() const
{
    uint32_t period = fSyncParams.availability_window_period;
    if (period < 4 || period > 64)
        period = 16;
    return (uint64_t)period * presenceMode() * 1024;
}

bool AirportItlwmAWDLInterface::
isSyncMaster() const
{
    static const ether_addr zero = {};
    return !memcmp(&fSyncMaster, &zero, sizeof(zero)) || !memcmp(&fSyncMaster, &fMac, sizeof(fMac));
}

void AirportItlwmAWDLInterface::
startAWTimeline()
{
    if (fAWRunning || !fAWTimer || !awTimelineAllowed())
        return;
    uint64_t now = hostUs();
    uint16_t pm = presenceMode();
    // Free run until a master is programmed; any non-zero start works, the
    // peer manager only flags a sequence number stuck at 0.
    fRefUs = now;
    fRefSeq = (uint16_t)(((now / 1000) & 0x3fff) / pm * pm + pm);
    fLastValid = false;
    fInAW = false;
    fAWRunning = true;
    fHuntStartUs = 0;               // every AWDL session gets its own hunt window
    publishAWDLAddress();
    AWDL_LOG("seq %u slot %llu us", fRefSeq, slotUs());
    scheduleNextAW(now);
    if (awDataAllowed() && awListenAllowed())
        scheduleAlignedListen(now);
    publishSyncState();
}

void AirportItlwmAWDLInterface::
stopAWTimeline()
{
    if (!fAWRunning)
        return;
    fAWRunning = false;
    if (fAWTimer)
        fAWTimer->cancelTimeout();
    fListenAlignedActive = false;
    fPeerValid = false;
    fPeerChanValid = false;
    if (fController) {
        if (ieee80211com *ic = fController->fHalService->get80211Controller()) {
            ic->ic_awdl_want_data = 0;
            if (ic->ic_awdl_stop)
                ic->ic_awdl_stop(ic);
        }
    }
    if (fListenTimer)
        fListenTimer->cancelTimeout();
    fTxPending = 0;
    fTxDwellEndUs = 0;
    if (fTxTimer)
        fTxTimer->cancelTimeout();
    // Nothing can leave without the timeline; do not send stale frames later.
    while (fDataQCount) {
        mbuf_freem(fDataQ[fDataQHead]);
        fDataQ[fDataQHead] = nullptr;
        fDataQHead = (fDataQHead + 1) % kDataQueueLen;
        fDataQCount--;
    }
    clearLowLatencyData();
    if (fInAW) {
        AWDLAWEndEvent end = {};
        end.endUsBE = OSSwapHostToBigInt32((uint32_t)hostUs());
        postMessage(kAWDLMsgAWEnd, &end, sizeof(end), true);
    }
    fInAW = false;
    AWDL_LOG("starts %llu syncs %llu", fAWStarts, fSyncUpdates);
    publishSyncState();
}

void AirportItlwmAWDLInterface::
teardownAW()
{
    if (fAWRunning && fController) {
        ieee80211com *ic = fController->fHalService->get80211Controller();
        if (ic->ic_awdl_stop)
            ic->ic_awdl_stop(ic);
    }
    fAWRunning = false;
    fListenAlignedActive = false;
    if (fListenTimer) {
        fListenTimer->cancelTimeout();
        if (fListenTimer->getWorkLoop())
            fListenTimer->getWorkLoop()->removeEventSource(fListenTimer);
        OSSafeReleaseNULL(fListenTimer);
    }
    fTxPending = 0;
    fTxDwellEndUs = 0;
    while (fDataQCount) {
        mbuf_freem(fDataQ[fDataQHead]);
        fDataQ[fDataQHead] = nullptr;
        fDataQHead = (fDataQHead + 1) % kDataQueueLen;
        fDataQCount--;
    }
    clearLowLatencyData();
    if (fTxTimer) {
        fTxTimer->cancelTimeout();
        if (fTxTimer->getWorkLoop())
            fTxTimer->getWorkLoop()->removeEventSource(fTxTimer);
        OSSafeReleaseNULL(fTxTimer);
    }
    if (fPeerCacheTimer) {
        fPeerCacheTimer->cancelTimeout();
        if (fPeerCacheTimer->getWorkLoop())
            fPeerCacheTimer->getWorkLoop()->removeEventSource(fPeerCacheTimer);
        OSSafeReleaseNULL(fPeerCacheTimer);
    }
    fPeerCacheDoneCount = 0;
    if (fUmiAckTimer) {
        fUmiAckTimer->cancelTimeout();
        if (fUmiAckTimer->getWorkLoop())
            fUmiAckTimer->getWorkLoop()->removeEventSource(fUmiAckTimer);
        OSSafeReleaseNULL(fUmiAckTimer);
    }
    fUmiAckCount = 0;
    if (!fAWTimer)
        return;
    fAWTimer->cancelTimeout();
    if (fAWTimer->getWorkLoop())
        fAWTimer->getWorkLoop()->removeEventSource(fAWTimer);
    OSSafeReleaseNULL(fAWTimer);
}

void AirportItlwmAWDLInterface::
scheduleNextAW(uint64_t nowUs)
{
    if (!fAWRunning || !fAWTimer)
        return;
    int64_t slot = (int64_t)slotUs();
    int64_t since = (int64_t)nowUs - fRefUs + kTimerEarlyUs;
    int64_t k = since >= 0 ? since / slot + 1 : -((-since) / slot);
    int64_t when = fRefUs + k * slot;
    uint64_t abs;
    nanoseconds_to_absolutetime((uint64_t)when * 1000, &abs);
    fAWTimer->cancelTimeout();
    fAWTimer->wakeAtTime(abs);
}

void AirportItlwmAWDLInterface::
awTimerFired(OSObject *owner, IOTimerEventSource *)
{
    AirportItlwmAWDLInterface *that = OSDynamicCast(AirportItlwmAWDLInterface, owner);
    if (!that || !that->fAWRunning)
        return;
    that->postAWEvents();
    that->scheduleNextAW(hostUs());
}

void AirportItlwmAWDLInterface::
postAWEvents()
{
    uint64_t now = hostUs();
    int64_t slot = (int64_t)slotUs();
    uint16_t pm = presenceMode();
    int64_t since = (int64_t)now - fRefUs + kTimerEarlyUs;
    int64_t k = since >= 0 ? since / slot : -((-since + slot - 1) / slot);
    int64_t start = fRefUs + k * slot;
    uint16_t seq = (uint16_t)(fRefSeq + k * pm);
    // A re-sync can map this boundary to the slot we already reported.
    if (fLastValid && seq == fLastSeq)
        return;

    // The peer manager expects end then start (it clears its "in AW" and
    // back-to-back flags on the end event).
    if (fInAW) {
        AWDLAWEndEvent end = {};
        end.endUsBE = OSSwapHostToBigInt32((uint32_t)start);
        postMessage(kAWDLMsgAWEnd, &end, sizeof(end), true);
        fInAW = false;
    }

    // Channel 0 in the sequence is an idle slot. The firmware sends no AW start
    // there: sanityCheckAWStart counts the skipped slots and only complains
    // about skipped slots that have a channel.
    unsigned count = fChanSeqValid ? fChanSeq[kChanSeqCount] : 0;
    const ieee80211com *ic = fController->fHalService->get80211Controller();
    if (ic->ic_awdl_home_chan && ic->ic_awdl_listener_chan != ic->ic_awdl_home_chan) {
        if (!fListenAlignedActive && awListenAllowed())
            scheduleAlignedListen(now);
        return;
    }
    const apple80211_channel *slotChan = count ?
        (const apple80211_channel *)(fChanSeq + kChanSeqEntries) + (seq / pm) % count : nullptr;
    if (slotChan && !slotChan->channel) {
        fLastValid = true;
        fLastSeq = seq;
        return;
    }

    AWDLAWStartEvent ev = {};
    ev.seq = seq;
    ev.role = isSyncMaster() ? 1 : 2;
    const ether_addr &master = isSyncMaster() ? fMac : fSyncMaster;
    memcpy(ev.master, &master, sizeof(ev.master));
    memcpy(ev.topMaster, &master, sizeof(ev.topMaster));
    ev.startUsBE = OSSwapHostToBigInt32((uint32_t)start);
    ev.chan.version = APPLE80211_VERSION;
    if (slotChan) {
        ev.chan.channel = ownChannelAt(seq);
        ev.chan.flags = slotChan->flags;
        if (ic->ic_awdl_listener_chan)
            ev.chan.flags = (ev.chan.channel <= 14 ? APPLE80211_C_FLAG_2GHZ :
                APPLE80211_C_FLAG_5GHZ) | APPLE80211_C_FLAG_20MHZ;
    }
    if (!ev.chan.channel) {
        ev.chan.channel = 6;
        ev.chan.flags = APPLE80211_C_FLAG_2GHZ | APPLE80211_C_FLAG_20MHZ;
    }
    postMessage(kAWDLMsgAWStart, &ev, sizeof(ev), true);
    maybeListen(ev.chan.channel, seq);

    fInAW = true;
    fLastValid = true;
    fLastSeq = seq;
    fLastStartUs = start;
    fAWStarts++;
    if (fAWStarts <= 4 || (fAWStarts & 0x3f) == 0)
        publishSyncState();
}

// Follow the master's timeline from its Synchronization Parameters TLV, as
// OWL does (seemoo-lab/owl src/sync.c, rx.c): the next extended AW (presence
// mode AWs) starts tx_down_counter TU after the frame, and its sequence number
// is next_aw_seq rounded down to the presence mode, plus one presence mode
// (same arithmetic as IO80211AWDLPeer::calculatePeerTimeDiff).
void AirportItlwmAWDLInterface::
syncFromFrame(const uint8_t *af, size_t len, const uint8_t *src, uint64_t rxUs)
{
    if (!fAWRunning || isSyncMaster() || memcmp(src, &fSyncMaster, sizeof(fSyncMaster)))
        return;
    size_t off = kAWDLFixedHeaderLen;
    while (off + 3 <= len) {
        uint8_t type = af[off];
        uint16_t tlen = af[off + 1] | (af[off + 2] << 8);
        const uint8_t *v = af + off + 3;
        if (off + 3 + tlen > len)
            return;
        if (type == kAWDLSyncParamsTLV) {
            if (tlen < 31)
                return;
            uint16_t down = v[1] | (v[2] << 8);
            uint16_t tlvPm = v[27];
            uint16_t nextSeq = v[29] | (v[30] << 8);
            uint16_t pm = presenceMode();
            if (tlvPm && tlvPm != pm)
                setProperty("AWDLMasterPresenceMode", tlvPm, 32);
            // Fixed header bytes 8..11 / 12..15 are phy_tx / target_tx (us,
            // little-endian). Their difference may be the delay between building
            // the frame (tx_down_counter) and sending it [inferred]; recorded
            // for now, not applied.
            uint32_t phyTx = af[8] | (af[9] << 8) | (af[10] << 16) | ((uint32_t)af[11] << 24);
            uint32_t targetTx = af[12] | (af[13] << 8) | (af[14] << 16) | ((uint32_t)af[15] << 24);
            int64_t txDelay = (int32_t)(phyTx - targetTx);
            if (txDelay < 0 || txDelay > 100000)
                txDelay = 0;
            int64_t boundary = (int64_t)rxUs + (int64_t)down * 1024;
            uint16_t seq = (uint16_t)(nextSeq / pm * pm + pm);

            // Error of our timeline at that boundary.
            int64_t slot = (int64_t)slotUs();
            int64_t d = boundary - fRefUs;
            int64_t k = d >= 0 ? (d + slot / 2) / slot : -((-d + slot / 2) / slot);
            int64_t err = d - k * slot;
            int16_t seqErr = (int16_t)(seq - (uint16_t)(fRefSeq + k * pm));
            fLastSyncErrUs = err;
            setProperty("AWDLSyncTxDelayUs", (uint64_t)txDelay, 32);

            // The peer manager tolerates +/-10 ms between AW starts. Ignore a
            // single large outlier (rx latency spike); re-anchor only when the
            // error persists, the sequence differs or we are not locked yet.
            const int64_t kOutlierUs = 6000;
            bool large = err > kOutlierUs || err < -kOutlierUs;
            if (fSyncUpdates && !seqErr && large && ++fSyncOutliers < 3) {
                setProperty("AWDLSyncOutliers", fSyncOutliersTotal + 1, 64);
                fSyncOutliersTotal++;
                return;
            }
            fSyncOutliers = 0;

            fRefUs = boundary;
            fRefSeq = seq;
            fSyncUpdates++;
            if (fSyncUpdates <= 4 || (fSyncUpdates & 0x3f) == 0 || seqErr) {
                setProperty("AWDLSyncSeqErr", (uint64_t)(int64_t)seqErr, 32);
                publishSyncState();
            }
            scheduleNextAW(hostUs());
            return;
        }
        off += 3 + tlen;
    }
}

// 2b: the station stays on the AP channel. When that is not the channel of
// this AW slot, ask the firmware for a short off-channel visit (iwx listen
// scan) so AWDL frames sent in the AW can be heard. Only every
// kListenEveryNthSlot-th such slot, to keep the station link usable.
// Defaults; both can be changed at run time (setProperties). A new listen is
// refused while one is pending or running, which bounds the duty cycle.
enum : unsigned {
    kListenDwellTU = 160,
    kListenEveryNthSlot = 1,
    kListenDwellMinTU = 16,
    kListenDwellMaxTU = 1000,
};

IOReturn AirportItlwmAWDLInterface::
setProperties(OSObject *properties)
{
    OSDictionary *d = OSDynamicCast(OSDictionary, properties);
    // Only while the experimental listen is on, so a stock -itlawdl boot
    // exposes no new writable state.
    if (!d || !awListenAllowed())
        return IO80211AWDLProtocol::setProperties(properties);
    bool handled = false;
    if (OSNumber *n = OSDynamicCast(OSNumber, d->getObject("AWDLListenDwellTU"))) {
        uint32_t v = n->unsigned32BitValue();
        if (v < kListenDwellMinTU || v > kListenDwellMaxTU)
            return kIOReturnBadArgument;
        fListenDwellTU = v;
        handled = true;
    }
    if (OSNumber *n = OSDynamicCast(OSNumber, d->getObject("AWDLListenEveryNth"))) {
        uint32_t v = n->unsigned32BitValue();
        if (v < 1 || v > 64)
            return kIOReturnBadArgument;
        fListenEveryNth = v;
        handled = true;
    }
    if (OSNumber *n = OSDynamicCast(OSNumber, d->getObject("AWDLListenAlign"))) {
        // Called on a user thread; the timers and peer state belong to the work loop.
        uintptr_t align = n->unsigned32BitValue() != 0;
        handled = true;
        if (_fCommandGate)
            _fCommandGate->runAction([](OSObject *, void *self, void *value, void *, void *) -> IOReturn {
                auto that = static_cast<AirportItlwmAWDLInterface *>(self);
                that->fListenAlign = (uint32_t)(uintptr_t)value;
                if (!that->fListenAlign) {
                    that->fListenAlignedActive = false;
                    if (that->fListenTimer)
                        that->fListenTimer->cancelTimeout();
                } else if (!that->fListenAlignedActive) {
                    that->scheduleAlignedListen(hostUs());
                }
                return kIOReturnSuccess;
            }, this, (void *)align);
    }
    if (!handled)
        return IO80211AWDLProtocol::setProperties(properties);
    setProperty("AWDLListenDwellTU", fListenDwellTU, 32);
    setProperty("AWDLListenEveryNth", fListenEveryNth, 32);
    setProperty("AWDLListenAlign", fListenAlign, 32);
    // Counters are published in batches; this gives callers an exact snapshot.
    publishListenState();
    AWDL_LOG("listen dwell %u TU every %u align %u", fListenDwellTU, fListenEveryNth, fListenAlign);
    return kIOReturnSuccess;
}

void AirportItlwmAWDLInterface::
publishListenState()
{
    {
        // "mac frames maxGapMs over6s ageMs" for each transmitter heard recently.
        char g[512];
        size_t o = 0;
        uint64_t now = hostUs();
        for (unsigned i = 0; i < kRxGapSlots && o < sizeof(g) - 64; i++) {
            const RxGap &e = fRxGap[i];
            if (!e.lastUs || now - e.lastUs > 120000000)
                continue;
            o += snprintf(g + o, sizeof(g) - o, "%02x%02x %u %llu %u %llu; ", e.mac[4], e.mac[5], e.count,
                          e.maxGapUs / 1000, e.over6s, (now - e.lastUs) / 1000);
        }
        g[o] = 0;
        setProperty("AWDLRxGap", g);
    }

    setProperty("AWDLRxChan6", fRxAFChan6, 64);
    setProperty("AWDLRxChan44", fRxAFChan44, 64);
    setProperty("AWDLRxChan149", fRxAFChan149, 64);
    setProperty("AWDLRxChanOther", fRxAFChanOther, 64);
    setProperty("AWDLListenStarted", fListenStarted, 64);
    setProperty("AWDLListenBusy", fListenBusy, 64);
    setProperty("AWDLListenAligned", fListenAligned, 64);
    setProperty("AWDLListenNoSlot", fListenNoSlot, 64);
    setProperty("AWDLListenHunt", fListenHunt, 64);
    setProperty("AWDLListenAlignedActive", fListenAlignedActive);
    setProperty("AWDLPeerUpdates", fPeerUpdates, 64);
    setProperty("AWDLPeerPredHit", fPeerPredHit, 64);
    setProperty("AWDLPeerPredMiss", fPeerPredMiss, 64);
    setProperty("AWDLPeerErrUs", (uint64_t)(fPeerLastErrUs < 0 ? -fPeerLastErrUs : fPeerLastErrUs), 32);
    if (fPeerChanValid) {
        char seq[16 * 4 + 1] = {};
        for (unsigned i = 0, n = 0; i < 16; i++)
            n += snprintf(seq + n, sizeof(seq) - n, " %u", fPeerChan[i]);
        setProperty("AWDLPeerChanSeq", seq);
    }
    ieee80211com *ic = fController ? fController->fHalService->get80211Controller() : nullptr;
    if (!ic)
        return;
    setProperty("AWDLListenDone", ic->ic_awdl_listen_done, 32);
    setProperty("AWDLListenRxAll", ic->ic_awdl_listen_rx_all, 32);
    setProperty("AWDLListenRxBeacon", ic->ic_awdl_listen_rx_beacon, 32);
    setProperty("AWDLListenRxAction", ic->ic_awdl_listen_rx_action, 32);
    {
        // Frame control byte (type/subtype) : count, for every kind of frame heard on the listen channel.
        char fc[64 * 12 + 1];
        size_t o = 0;
        for (unsigned i = 0; i < 64 && o + 12 < sizeof(fc); i++)
            if (ic->ic_awdl_rx_fc[i])
                o += snprintf(fc + o, sizeof(fc) - o, "%02x:%u ", ((i >> 4) << 2) | ((i & 15) << 4), ic->ic_awdl_rx_fc[i]);
        fc[o] = 0;
        setProperty("AWDLRxFc", fc);
        static const char *const names[3] = { "AWDLRxLastAction", "AWDLRxLastProbeReq", "AWDLRxLastOther" };
        for (unsigned k = 0; k < 3; k++) {
            char hex[32 * 2 + 1];
            for (unsigned i = 0; i < ic->ic_awdl_rx_last_len[k]; i++)
                snprintf(hex + i * 2, 3, "%02x", ic->ic_awdl_rx_last_hdr[k][i]);
            hex[ic->ic_awdl_rx_last_len[k] * 2] = 0;
            setProperty(names[k], hex);
        }
    }
    setProperty("AWDLRocSent", ic->ic_awdl_roc_sent, 32);
    setProperty("AWDLRocRespErr", ic->ic_awdl_roc_resp_err, 32);
    setProperty("AWDLRocStarted", ic->ic_awdl_roc_started, 32);
    setProperty("AWDLRocEnded", ic->ic_awdl_roc_ended, 32);
    setProperty("AWDLRocNotScheduled", ic->ic_awdl_roc_not_sched, 32);
    setProperty("AWDLRocStartFail", ic->ic_awdl_roc_start_fail, 32);
    setProperty("AWDLRocEndFail", ic->ic_awdl_roc_end_fail, 32);
    setProperty("AWDLRocEndFailShort", ic->ic_awdl_roc_end_fail_short, 32);
    setProperty("AWDLRocEndFailLastUs", ic->ic_awdl_roc_end_fail_last_us, 32);
    setProperty("AWDLRocChanChanges", ic->ic_awdl_roc_chan_changes, 32);
    setProperty("AWDLRocBandChanges", ic->ic_awdl_roc_band_changes, 32);
    char per[96];
    snprintf(per, sizeof(per), "6:%llu/%llu 44:%llu/%llu 149:%llu/%llu 161:%llu/%llu",
             fListenOnChan[0], fRxAFChan6, fListenOnChan[1], fRxAFChan44,
             fListenOnChan[2], fRxAFChan149, fListenOnChan[3], fRxAFChan161);
    setProperty("AWDLListenByChan", per);
    if (awTxAllowed())
        publishTxState();
    if (awDataAllowed())
        publishDataState();
    setProperty("AWDLRocStaErr", ic->ic_awdl_roc_sta_err, 32);
    setProperty("AWDLRocStaStage", ic->ic_awdl_roc_sta_stage, 32);
    setProperty("AWDLRocStaCode", ic->ic_awdl_roc_sta_code, 32);
    setProperty("AWDLRocQueue", ic->ic_awdl_roc_qid, 32);
    setProperty("AWDLRocLastUs", ic->ic_awdl_roc_last_us, 32);
    setProperty("AWDLRocMinUs", ic->ic_awdl_roc_min_us, 32);
    setProperty("AWDLRocMaxUs", ic->ic_awdl_roc_max_us, 32);
    setProperty("AWDLRocDelayUs", ic->ic_awdl_roc_delay_us, 32);
    setProperty("AWDLRocRxTarget", ic->ic_awdl_roc_rx_target, 32);
    setProperty("AWDLRocRxHome", ic->ic_awdl_roc_rx_home, 32);
    setProperty("AWDLRocRxOther", ic->ic_awdl_roc_rx_other, 32);
    setProperty("AWDLRocRxLastChannel", ic->ic_awdl_roc_rx_last_chan, 32);
}

void AirportItlwmAWDLInterface::
maybeListen(uint32_t slotChannel, uint16_t seq)
{
    if (!awListenAllowed() || !fController || !slotChannel)
        return;
    ieee80211com *ic = fController->fHalService->get80211Controller();
    if (!ic || !ic->ic_awdl_listen || ic->ic_state != IEEE80211_S_RUN || !ic->ic_bss)
        return;
    unsigned home = ieee80211_chan2ieee(ic, ic->ic_bss->ni_chan);
    if (unsigned forced = awListenChannelOverride())
        slotChannel = forced;
    if (ic->ic_awdl_home_chan) {
        if (home != ic->ic_awdl_home_chan)
            return;
        slotChannel = home;
    } else if (home == slotChannel) {
        fListenSkippedHome++;
        return;
    }
    // The peer's timeline is known: the listen timer picks the moments instead.
    if (fListenAlignedActive)
        return;
    if (++fListenSlot % fListenEveryNth)
        return;
    startListen(slotChannel, fListenDwellTU, home, false);
}

void AirportItlwmAWDLInterface::
startListen(unsigned chan, uint32_t dwellTU, unsigned home, bool aligned)
{
    ieee80211com *ic = fController->fHalService->get80211Controller();
    int err = ic->ic_awdl_listen(ic, (uint8_t)chan, (uint16_t)dwellTU);
    if (err) {
        fListenBusy++;
    } else {
        fListenStarted++;
        if (aligned)
            fListenAligned++;
        int idx = chan == 6 ? 0 : chan == 44 ? 1 : chan == 149 ? 2 : chan == 161 ? 3 : -1;
        if (idx >= 0)
            fListenOnChan[idx]++;
    }
    if (((fListenStarted + fListenBusy) & 0x1f) == 1) {
        publishListenState();
        setProperty("AWDLListenHomeChannel", home, 32);
        setProperty("AWDLListenChannel", chan, 32);
        setProperty("AWDLListenLastErr", err, 32);
    }
}

// A listen goes out this long before the slot: the firmware took about 4.4 ms
// from the command to its START notification.
static const int64_t kListenLeadUs = 6000;
// Without a frame from the peer for this long, go back to listening blindly.
static const uint64_t kPeerStaleUs = 30000000;
// Hunt: each social channel for this long, dwell and pause between listens. The pause keeps
// the station link alive (a beacon every 100 TU still gets through). It only runs for the
// first kHuntWindowUs of each AWDL session (counted from its first listen), so a diagnostic
// boot-arg left in place does not leave the station link at 60 % airtime for good. It used to
// count from the first listen after boot, and a test started later never hunted.
static const uint64_t kHuntWindowUs = 600000000;
static const uint64_t kHuntStepUs = 3000000;
static const uint32_t kHuntDwellTU = 100;
static const int64_t kHuntGapUs = 60000;
static const uint64_t kPeerReplaceUs = 10000000;

static int64_t floorDiv(int64_t a, int64_t b)
{
    return a >= 0 ? a / b : -((-a + b - 1) / b);
}

// Channel sequence as in a TLV 18 value (seemoo-lab/owl rx.c): count - 1,
// encoding, duplicate count, step count, fill channel (2), then the entries.
bool AirportItlwmAWDLInterface::
parsePeerChanSeq(const uint8_t *v, size_t len)
{
    if (len < 6 || v[0] + 1 != 16)
        return false;
    size_t size = v[1] == 0 ? 1 : (v[1] == 1 || v[1] == 3) ? 2 : 0;
    if (!size || len < 6 + 16 * size)
        return false;
    // Legacy keeps the channel in the second byte, op class in the first.
    size_t at = v[1] == 1 ? 1 : 0;
    for (unsigned i = 0; i < 16; i++)
        fPeerChan[i] = v[6 + i * size + at];
    fPeerChanValid = true;
    return true;
}

unsigned AirportItlwmAWDLInterface::
peerChannelAt(uint64_t us) const
{
    if (!fPeerValid || !fPeerChanValid || !fPeerSlotUs || !fPeerPm)
        return 0;
    int64_t k = floorDiv((int64_t)us - fPeerRefUs, fPeerSlotUs);
    uint16_t seq = (uint16_t)(fPeerRefSeq + k * fPeerPm);
    return fPeerChan[(seq / fPeerPm) % 16];
}

// Keep the timeline of the peer we hear, whoever our sync master is: when we
// are master ourselves syncFromFrame learns nothing about it.
void AirportItlwmAWDLInterface::
trackPeer(const uint8_t *af, size_t len, const uint8_t *src, uint64_t rxUs, unsigned rxChan)
{
    if (!fAWRunning || !awListenAllowed())
        return;
    if (fPeerValid && memcmp(src, &fPeerMac, sizeof(fPeerMac))) {
        if ((int64_t)(rxUs - fPeerSeenUs) < (int64_t)kPeerReplaceUs)
            return;
        fPeerValid = false;
        fPeerChanValid = false;
    }
    // Checks the slot-to-channel mapping, which is inferred for a peer.
    if (unsigned predicted = peerChannelAt(rxUs)) {
        if (predicted == rxChan)
            fPeerPredHit++;
        else
            fPeerPredMiss++;
    }
    size_t off = kAWDLFixedHeaderLen;
    bool sync = false;
    while (off + 3 <= len) {
        uint8_t type = af[off];
        uint16_t tlen = af[off + 1] | (af[off + 2] << 8);
        const uint8_t *v = af + off + 3;
        if (off + 3 + tlen > len)
            break;
        if (type == kAWDLSyncParamsTLV && tlen >= 31) {
            uint16_t down = v[1] | (v[2] << 8);
            uint16_t period = v[5] | (v[6] << 8);
            uint16_t pm = v[27];
            uint16_t nextSeq = v[29] | (v[30] << 8);
            if (period >= 4 && period <= 64 && pm >= 1 && pm <= 16) {
                int64_t boundary = (int64_t)rxUs + (int64_t)down * 1024;
                uint32_t slot = (uint32_t)period * pm * 1024;
                if (fPeerValid && fPeerSlotUs == slot) {
                    int64_t d = boundary - fPeerRefUs;
                    fPeerLastErrUs = d - floorDiv(d + slot / 2, slot) * slot;
                }
                fPeerRefUs = boundary;
                fPeerRefSeq = (uint16_t)(nextSeq / pm * pm + pm);
                fPeerPm = pm;
                fPeerSlotUs = slot;
                sync = true;
            }
            if (tlen > 33)
                parsePeerChanSeq(v + 33, tlen - 33);
        } else if (type == kAWDLChanSeqTLV) {
            parsePeerChanSeq(v, tlen);
        }
        off += 3 + tlen;
    }
    if (!sync)
        return;
    memcpy(&fPeerMac, src, sizeof(fPeerMac));
    fPeerValid = true;
    fPeerSeenUs = rxUs;
    fPeerUpdates++;
    if (!fListenAlignedActive)
        scheduleAlignedListen(hostUs());
}

// Arm the listen timer for the next run of peer slots we should visit.
uint8_t AirportItlwmAWDLInterface::
ownChannelAt(uint16_t seq) const
{
    unsigned count = fChanSeqValid ? fChanSeq[kChanSeqCount] : 0;
    if (!count)
        return 0;
    uint16_t pm = presenceMode();
    const apple80211_channel *c = (const apple80211_channel *)(fChanSeq + kChanSeqEntries) + (seq / pm) % count;
    const ieee80211com *ic = fController ? fController->fHalService->get80211Controller() : nullptr;
    // A standing link cannot follow the system's hopping sequence. Keep idle slots.
    if (c->channel && ic && ic->ic_awdl_listener_chan)
        return ic->ic_awdl_listener_chan;
    if (c->channel && ic && ic->ic_awdl_home_chan)
        return ic->ic_awdl_home_chan;
    return (uint8_t)c->channel;
}

// Next slot of our own timeline whose channel is not 0 (0 = not on AWDL, the
// firmware stays on the infra channel and posts no AW start) and is not the
// station channel. Dwell for the run of equal-channel slots.
bool AirportItlwmAWDLInterface::
scheduleOwnListen(uint64_t afterUs)
{
    if (!fListenTimer || !fAWRunning || !fChanSeqValid || !fController)
        return false;
    ieee80211com *ic = fController->fHalService->get80211Controller();
    if (!ic || ic->ic_state != IEEE80211_S_RUN || !ic->ic_bss)
        return false;
    unsigned home = ieee80211_chan2ieee(ic, ic->ic_bss->ni_chan);
    const bool homeOnly = ic->ic_awdl_home_chan != 0;
    if (homeOnly && home != ic->ic_awdl_home_chan)
        return false;
    uint64_t now = hostUs();
    // Data frames are only accepted while a peer is around (see ic_awdl_want_data);
    // the P2P MAC picks the change up before the next dwell.
    ic->ic_awdl_want_data = fPeerValid && now - fPeerSeenUs <= kPeerStaleUs;
    uint64_t from = afterUs > now ? afterUs : now;
    if (awHuntAllowed() && !fHuntStartUs)
        fHuntStartUs = now;
    if (awHuntAllowed() && !ic->ic_awdl_want_data && now - fHuntStartUs < kHuntWindowUs) {
        // Channel 6 is the rendezvous every AWDL node visits (slot 9 of its sequence) and the one
        // channel Intel firmware transmits on reliably, so it gets half of the time.
        static const uint8_t kSocial[4] = { 6, 149, 6, 44 };
        const uint8_t listener = ic->ic_awdl_listener_chan;
        uint8_t list[4];
        unsigned n = 0;
        for (unsigned i = 0; i < 4; i++)
            if (kSocial[i] != home)
                list[n++] = kSocial[i];
        fListenNextChan = homeOnly ? home :
            listener ? listener : list[(now / kHuntStepUs) % n];
        fListenNextDwellTU = kHuntDwellTU;
        // On hardware an iPhone's unicast frames to awdl0 arrived while none of its broadcast
        // PSF/MIF did, so the firmware filter drops broadcast frames that carry the AWDL BSSID.
        // The P2P MAC is promiscuous (-itlawdlpromisc) from the first listen, not only once a
        // peer is known, to let them through.
        ic->ic_awdl_want_data = 1;
        uint64_t abs;
        nanoseconds_to_absolutetime((from + kHuntGapUs) * 1000, &abs);
        fListenTimer->cancelTimeout();
        fListenTimer->wakeAtTime(abs);
        fListenAlignedActive = true;
        fListenHunt++;
        return true;
    }
    int64_t slot = (int64_t)slotUs();
    uint16_t pm = presenceMode();
    int64_t k = floorDiv((int64_t)from + kListenLeadUs - fRefUs, slot) + 1;
    // The peer we track (the phone during AirDrop) must be heard at least every
    // few seconds or IO80211Family drops it from NDP (lastAF ~6 s). When our own
    // slot keeps us home (or off AWDL) but the peer's slot at that time is on a
    // non-home channel, listen there instead (on unless -itlawdlnopeerlisten).
    bool usePeer = awPeerListenAllowed() && fPeerValid && fPeerChanValid && fPeerSlotUs &&
        fPeerPm && now - fPeerSeenUs <= kPeerStaleUs;
    for (int i = 0; i < 32; i++, k++) {
        uint16_t seq = (uint16_t)(fRefSeq + k * pm);
        uint8_t chan = ownChannelAt(seq);
        bool fromPeer = false;
        if (homeOnly && usePeer &&
            peerChannelAt((uint64_t)(fRefUs + k * slot + slot / 2)) != home)
            continue;
        if (!homeOnly && (!chan || chan == home) && usePeer) {
            unsigned pc = peerChannelAt((uint64_t)(fRefUs + k * slot + slot / 2));
            if (pc && pc != home) {
                chan = (uint8_t)pc;
                fromPeer = true;
            }
        }
        if (!chan || (!homeOnly && chan == home) ||
            (ic->ic_awdl_listener_chan && chan != ic->ic_awdl_listener_chan))
            continue;
        unsigned run = 1;
        if (!fromPeer) {
            while (run < 16 && ownChannelAt((uint16_t)(seq + run * pm)) == chan)
                run++;
        }
        if (fromPeer)
            fListenPeerSlots++;
        uint32_t dwell = (uint32_t)((run * slot + kListenLeadUs) / 1024);
        if (dwell > fListenDwellTU)
            dwell = fListenDwellTU;
        fListenNextChan = chan;
        fListenNextDwellTU = dwell;
        uint64_t abs;
        nanoseconds_to_absolutetime((uint64_t)(fRefUs + k * slot - kListenLeadUs) * 1000, &abs);
        fListenTimer->cancelTimeout();
        fListenTimer->wakeAtTime(abs);
        fListenAlignedActive = true;
        return true;
    }
    fListenNoSlot++;
    return false;
}

void AirportItlwmAWDLInterface::
scheduleAlignedListen(uint64_t afterUs)
{
    fListenAlignedActive = false;
    if (awDataAllowed()) {
        scheduleOwnListen(afterUs);
        return;
    }
    if (!fListenTimer || !fListenAlign || !fAWRunning || !fPeerValid || !fPeerChanValid || !fController)
        return;
    uint64_t now = hostUs();
    if (now - fPeerSeenUs > kPeerStaleUs)
        return;
    ieee80211com *ic = fController->fHalService->get80211Controller();
    if (!ic || ic->ic_state != IEEE80211_S_RUN || !ic->ic_bss)
        return;
    unsigned home = ieee80211_chan2ieee(ic, ic->ic_bss->ni_chan);
    unsigned forced = awListenChannelOverride();
    int64_t slot = fPeerSlotUs;
    uint64_t from = afterUs > now ? afterUs : now;
    int64_t k = floorDiv((int64_t)from + kListenLeadUs - fPeerRefUs, slot) + 1;
    for (int i = 0; i < 32; i++, k++) {
        uint16_t seq = (uint16_t)(fPeerRefSeq + k * fPeerPm);
        unsigned chan = fPeerChan[(seq / fPeerPm) % 16];
        if (!chan || chan == home || (forced && chan != forced))
            continue;
        // Stay for the whole run of slots on this channel, up to the dwell limit.
        unsigned run = 1;
        while (run < 16 && fPeerChan[((seq / fPeerPm) + run) % 16] == chan)
            run++;
        uint32_t dwell = (uint32_t)((run * slot + kListenLeadUs) / 1024);
        if (dwell > fListenDwellTU)
            dwell = fListenDwellTU;
        fListenNextChan = (uint8_t)chan;
        fListenNextDwellTU = dwell;
        uint64_t abs;
        nanoseconds_to_absolutetime((uint64_t)(fPeerRefUs + k * slot - kListenLeadUs) * 1000, &abs);
        fListenTimer->cancelTimeout();
        fListenTimer->wakeAtTime(abs);
        fListenAlignedActive = true;
        return;
    }
    fListenNoSlot++;
}

void AirportItlwmAWDLInterface::
listenTimerFired(OSObject *owner, IOTimerEventSource *)
{
    AirportItlwmAWDLInterface *that = OSDynamicCast(AirportItlwmAWDLInterface, owner);
    if (!that || !that->fListenAlignedActive || !that->fAWRunning || !that->fController)
        return;
    uint64_t now = hostUs();
    uint32_t dwell = that->fListenNextDwellTU;
    ieee80211com *ic = that->fController->fHalService->get80211Controller();
    if (ic && ic->ic_awdl_listen && ic->ic_state == IEEE80211_S_RUN && ic->ic_bss) {
        unsigned home = ieee80211_chan2ieee(ic, ic->ic_bss->ni_chan);
        const bool matches = ic->ic_awdl_home_chan ?
            home == ic->ic_awdl_home_chan && that->fListenNextChan == home :
            home != that->fListenNextChan;
        if (matches && !(++that->fListenSlot % that->fListenEveryNth)) {
            uint64_t before = that->fListenStarted;
            that->startListen(that->fListenNextChan, dwell, home, true);
            if (that->fListenStarted != before) {
                if (awDataAllowed())
                    that->fListenOwn++;
                that->scheduleTx(that->fListenNextChan, now + kListenLeadUs,
                    now + (uint64_t)dwell * 1024);
            }
        }
    }
    that->scheduleAlignedListen(now + (uint64_t)dwell * 1024);
}

#pragma mark - stage 3 step 1: transmit (host side of the firmware PSF / MI engine)

// The peer manager hands down TLVs only (see updateSyncFrameTemplate). Broadcom firmware
// adds the 802.11 header, the AWDL fixed header, the synchronization
// parameters (TLV 4) and the channel sequence (TLV 18). We do the same and
// send during an aligned listen dwell, on the P2P station's queue, in the
// peer's AW: PSF at 1 ms, data at 4 ms and MI at its requested offset.
static const uint8_t kAWDLBssid[6] = { 0x00, 0x25, 0x00, 0xff, 0x94, 0x73 };
static const uint32_t kTxPSFAtUs = 1000;
static const uint32_t kTxDataAtUs = 4000;
static const uint32_t kTxDataDwellGuardUs = 2000;
enum : uint8_t {
    kTxPendingPSF = 1 << 0,
    kTxPendingData = 1 << 1,
    kTxPendingMI = 1 << 2,
    kAWDLSubtypePSF = 0,
    kAWDLSubtypeMI = 3,
};

void AirportItlwmAWDLInterface::
scheduleTx(uint8_t chan, uint64_t slotStartUs, uint64_t dwellEndUs)
{
    fTxDwellEndUs = 0;
    if (!awTxAllowed() || !fTxTimer || !fAWRunning)
        return;
    if (fAfTxMode == 3) {
        fTxSkipMode++;
        return;
    }
    fTxChan = chan;
    fTxSlotUs = slotStartUs;
    fTxDwellEndUs = dwellEndUs;
    fTxPending = kTxPendingPSF | kTxPendingData;
    if (fMITmplLen && fChanSeqValid) {
        const uint32_t count = fChanSeq[kChanSeqCount];
        const uint32_t step = static_cast<uint32_t>(fChanSeq[kChanSeqStep]) + 1;
        const int64_t k = floorDiv(static_cast<int64_t>(slotStartUs) - fRefUs,
            static_cast<int64_t>(slotUs()));
        const uint32_t awSequence = static_cast<uint16_t>(fRefSeq + k * step);
        const uint32_t slot = (awSequence % (count * step)) / step;
        // Tahoe 25G229 tests miBitmap with 1 << channel-sequence slot.
        fTxMISlot = static_cast<uint8_t>(MIN(slot, 15U));
        fTxPending |= kTxPendingMI;
    }
    scheduleTxTimer();
}

void AirportItlwmAWDLInterface::
scheduleTxTimer()
{
    uint64_t nextUs = 0xffffffffffffffffULL;
    if (fTxPending & kTxPendingPSF)
        nextUs = kTxPSFAtUs;
    if ((fTxPending & kTxPendingData) && kTxDataAtUs < nextUs)
        nextUs = kTxDataAtUs;
    if ((fTxPending & kTxPendingMI) && fTxMIOffsetUs < nextUs)
        nextUs = fTxMIOffsetUs;
    uint64_t abs;
    nanoseconds_to_absolutetime((fTxSlotUs + nextUs) * 1000, &abs);
    fTxTimer->cancelTimeout();
    fTxTimer->wakeAtTime(abs);
}

void AirportItlwmAWDLInterface::
txTimerFired(OSObject *owner, IOTimerEventSource *)
{
    AirportItlwmAWDLInterface *that = OSDynamicCast(AirportItlwmAWDLInterface, owner);
    if (!that || !that->fTxPending || !that->fAWRunning)
        return;
    uint64_t now = hostUs();
    uint64_t elapsed = now > that->fTxSlotUs ? now - that->fTxSlotUs : 0;
    if ((that->fTxPending & kTxPendingPSF) && elapsed >= kTxPSFAtUs) {
        if (that->fSyncTmplLen)
            that->sendAWDLFrame(false, that->fTxChan, nullptr, that->fSyncTmpl, that->fSyncTmplLen);
        that->fTxPending &= ~kTxPendingPSF;
    }
    if ((that->fTxPending & kTxPendingData) && elapsed >= kTxDataAtUs) {
        if (awDataAllowed()) {
            that->flushLowLatencyData();
            unsigned pc = that->peerChannelAt(now);
            if (!pc || pc == that->fTxChan)
                that->flushData();
            else if (that->fDataQCount)
                that->fDataTxNotCommon++;
        }
        that->fTxPending &= ~kTxPendingData;
    }
    if ((that->fTxPending & kTxPendingMI) && elapsed >= that->fTxMIOffsetUs) {
        if (that->fMITmplLen &&
            (that->fTxMIBitmap & (1U << that->fTxMISlot)))
            that->sendAWDLFrame(true, that->fTxChan, nullptr, that->fMITmpl, that->fMITmplLen);
        that->fTxPending &= ~kTxPendingMI;
    }
    if (that->fTxPending)
        that->scheduleTxTimer();
    if (((that->fTxPSF + that->fTxMI + that->fTxRefused) & 0x1f) <= 1)
        that->publishTxState();
}

// Sync parameters TLV as parsed by IO80211AWDLPeer::parseAwdlSyncParamsTlv
// (same layout as OWL's awdl_sync_params_tlv), times from our AW timeline.
size_t AirportItlwmAWDLInterface::
buildAWDLFrame(uint8_t *buf, size_t cap, bool mi, uint64_t nowUs,
    const uint8_t *destination, const uint8_t *payload, size_t payloadLen)
{
    const size_t tmplLen = payloadLen;
    if (tmplLen && !payload)
        return 0;
    // Channel entries are 2 bytes (op class encoding): the receiver's
    // IO80211AWDLPeerManager::isValidChanSeqTlv wants at least 2 * count + 2
    // bytes and drops the whole frame otherwise (iPhone: "bad chan seq tlv",
    // parse error reason 1, with the 1-byte encoding).
    const size_t syncLen = 3 + 33 + 6 + 32 + 2;
    const size_t seqLen = 3 + 6 + 32 + 3;
    size_t need = sizeof(struct ieee80211_frame) + kAWDLFixedHeaderLen + syncLen + seqLen + tmplLen;
    if (need > cap)
        return 0;
    memset(buf, 0, need);

    struct ieee80211_frame *wh = (struct ieee80211_frame *)buf;
    wh->i_fc[0] = IEEE80211_FC0_VERSION_0 | IEEE80211_FC0_TYPE_MGT | IEEE80211_FC0_SUBTYPE_ACTION;
    if (destination)
        memcpy(wh->i_addr1, destination, 6);
    else
        memset(wh->i_addr1, 0xff, 6);
    memcpy(wh->i_addr2, &fMac, 6);
    const ieee80211com *ic = fController ? fController->fHalService->get80211Controller() : nullptr;
    memcpy(wh->i_addr3, ic && ic->ic_awdl_bssid_valid ? ic->ic_awdl_bssid : kAWDLBssid, 6);
    uint16_t sc = (uint16_t)(fTxSeq++ << 4);
    wh->i_seq[0] = sc & 0xff;
    wh->i_seq[1] = sc >> 8;

    uint8_t *af = buf + sizeof(*wh);
    af[0] = 127;
    af[1] = 0x00; af[2] = 0x17; af[3] = 0xf2;
    af[4] = 8;
    af[5] = 0x10;           // version 1.0; IO80211 rejects 0 (actionFrameHasCorrectOUI)
    af[6] = mi ? kAWDLSubtypeMI : kAWDLSubtypePSF;
    af[7] = 0;
    uint32_t t = (uint32_t)nowUs;
    memcpy(af + 8, &t, 4);  // phy_tx
    memcpy(af + 12, &t, 4); // target_tx

    // Our timeline: the extended AW (slot) we are in and the next boundary.
    uint16_t pm = presenceMode();
    uint32_t period = fSyncParams.availability_window_period;
    if (period < 4 || period > 64)
        period = 16;
    int64_t slot = (int64_t)slotUs();
    int64_t k = floorDiv((int64_t)nowUs - fRefUs, slot);
    int64_t start = fRefUs + k * slot;
    uint16_t seq = (uint16_t)(fRefSeq + k * pm);
    uint16_t curAW = (uint16_t)(seq + ((int64_t)nowUs - start) / ((int64_t)period * 1024));
    uint16_t down = (uint16_t)((start + slot - (int64_t)nowUs + 1023) / 1024);
    unsigned count = fChanSeqValid ? fChanSeq[kChanSeqCount] : 0;
    const uint8_t listenerChan = ic ? ic->ic_awdl_listener_chan : 0;
    auto chanAt = [&](uint16_t s) -> uint8_t {
        if (!count)
            return listenerChan ? listenerChan : 6;
        return ownChannelAt(s);
    };
    const ether_addr &master = isSyncMaster() ? fMac : fSyncMaster;
    uint8_t masterChan = isSyncMaster() && listenerChan ? listenerChan :
        (fMasterChannel ? (uint8_t)fMasterChannel : 6);

    uint8_t *p = af + kAWDLFixedHeaderLen;
    auto put16 = [](uint8_t *q, uint16_t v) { q[0] = v & 0xff; q[1] = v >> 8; };
    // Global operating class of a 20 MHz channel (IEEE 802.11 Annex E).
    auto opClass = [](uint8_t c) -> uint8_t {
        if (!c) return 0;
        if (c <= 13) return 81;
        if (c <= 48) return 115;
        if (c <= 64) return 118;
        if (c <= 144) return 121;
        return 124;
    };
    auto chanSeq = [&](uint8_t *q) {
        q[0] = 15;          // count - 1
        q[1] = 3;           // op class encoding: channel, op class
        // Header bytes as AppleBCMWLANProximityInterface::buildAWDLChanSequence
        // takes them from the system's sequence: step +7, duplicate +8,
        // fill +9 (255 = 0xffff, repeat the current channel).
        q[2] = fChanSeqValid ? fChanSeq[8] : 0;
        q[3] = fChanSeqValid ? fChanSeq[kChanSeqStep] : (uint8_t)(pm - 1);
        uint8_t fill = fChanSeqValid ? fChanSeq[9] : 255;
        if (listenerChan && fill && fill != 255)
            fill = listenerChan;
        put16(q + 4, fill == 255 ? 0xffff : (uint16_t)(fill | (opClass(fill) << 8)));
        for (unsigned i = 0; i < 16; i++) {
            uint8_t c = chanAt((uint16_t)(i * pm));
            q[6 + 2 * i] = c;
            q[7 + 2 * i] = opClass(c);
        }
    };
    p[0] = kAWDLSyncParamsTLV;
    put16(p + 1, (uint16_t)(syncLen - 3));
    uint8_t *v = p + 3;
    v[0] = chanAt((uint16_t)(seq + pm));    // next AW channel
    put16(v + 1, down);                     // tx_down_counter
    v[3] = masterChan;
    v[4] = 0;                               // guard time
    put16(v + 5, (uint16_t)period);         // aw_period
    put16(v + 7, (uint16_t)(fSyncParams.synchronization_frame_period ?: IEEE80211_AWDL_DEFAULT_SYNC_PERIOD_TU));
    put16(v + 9, 0x1800);                   // flags, as OWL
    put16(v + 11, (uint16_t)period);        // aw_ext_length (16, else "Zero AWExt")
    put16(v + 13, (uint16_t)period);        // aw_com_length
    int64_t rem = (int64_t)period - (int64_t)((nowUs - start) / 1024);
    put16(v + 15, (uint16_t)(rem > 0 ? rem : 0));
    v[17] = v[18] = v[19] = v[20] = (uint8_t)(pm - 1);
    memcpy(v + 21, &master, 6);
    v[27] = (uint8_t)pm;
    put16(v + 29, curAW);                   // next_aw_seq
    put16(v + 31, curAW);                   // ap_alignment
    chanSeq(v + 33);
    p += syncLen;

    p[0] = kAWDLChanSeqTLV;
    put16(p + 1, (uint16_t)(seqLen - 3));
    chanSeq(p + 3);
    p += seqLen;

    if (tmplLen)
        memcpy(p, payload, tmplLen);
    return need;
}

int AirportItlwmAWDLInterface::
sendAWDLFrame(bool mi, uint8_t chan, const uint8_t *destination,
    const uint8_t *payload, size_t payloadLen)
{
    ieee80211com *ic = fController ? fController->fHalService->get80211Controller() : nullptr;
    if (!ic || !ic->ic_awdl_tx)
        return ENXIO;
    static uint8_t frame[2048];             // work loop only
    size_t len = buildAWDLFrame(frame, sizeof(frame), mi, hostUs(),
        destination, payload, payloadLen);
    if (!len) {
        fTxLastErr = 0x100;
        fTxRefused++;
        return EMSGSIZE;
    }
    int err = ic->ic_awdl_tx(ic, chan, frame, len);
    if (err) {
        fTxRefused++;
        fTxLastErr = err;
        return err;
    }
    fTxLastLen = (uint32_t)len;
    if (mi)
        fTxMI++;
    else
        fTxPSF++;
    return 0;
}

void AirportItlwmAWDLInterface::
enqueueData(mbuf_t m)
{
    if (!m)
        return;
    if (fDataQCount >= kDataQueueLen) {
        fDataTxDropFull++;
        mbuf_freem(m);
        return;
    }
    fDataQ[(fDataQHead + fDataQCount) % kDataQueueLen] = m;
    fDataQCount++;
    fDataTxQueued++;

    // The timed flush already serves bursts queued at the start of a slot.
    // If one packet arrives later (often a TCP ACK), use the remainder of
    // the current common dwell without changing the radio's schedule.
    if (fDataQCount != 1 || !fAWRunning || !fTxDwellEndUs ||
        (fTxPending & kTxPendingData) || fAfTxMode == 3)
        return;
    const uint64_t now = hostUs();
    if (now < fTxSlotUs + kTxDataAtUs || now + kTxDataDwellGuardUs >= fTxDwellEndUs ||
        !fTxChan || now - fPeerSeenUs > kPeerStaleUs ||
        peerChannelAt(now) != fTxChan ||
        peerChannelAt(now + kTxDataDwellGuardUs) != fTxChan)
        return;
    const uint64_t sent = fDataTx;
    flushData();
    fDataTxWithinDwell += fDataTx - sent;
}

// Ethernet -> 802.11 data (no QoS) + LLC/SNAP 00:17:f2 0x0800 + AWDL data
// header (0x0403 LE, sequence LE, pad, ethertype BE) + payload, as OWL's
// awdl_init_full_data_frame. Sent on the P2P queue while dwelling on the
// peer's channel; frames that cannot go out yet stay queued.
void AirportItlwmAWDLInterface::
flushData()
{
    ieee80211com *ic = fController ? fController->fHalService->get80211Controller() : nullptr;
    if (!ic || !ic->ic_awdl_tx)
        return;
    static uint8_t frame[2048];             // work loop only
    while (fDataQCount) {
        mbuf_t m = fDataQ[fDataQHead];
        size_t plen = mbuf_pkthdr_len(m);
        const size_t hdr = sizeof(struct ieee80211_frame), llc = 8, ad = 8;
        if (plen < 14 || hdr + llc + ad + plen - 14 > sizeof(frame)) {
            fDataTxErr++;
        } else {
            uint8_t eh[14];
            mbuf_copydata(m, 0, sizeof(eh), eh);
            memset(frame, 0, hdr + llc + ad);
            struct ieee80211_frame *wh = (struct ieee80211_frame *)frame;
            wh->i_fc[0] = IEEE80211_FC0_VERSION_0 | IEEE80211_FC0_TYPE_DATA | IEEE80211_FC0_SUBTYPE_DATA;
            memcpy(wh->i_addr1, eh, 6);
            memcpy(wh->i_addr2, &fMac, 6);
            memcpy(wh->i_addr3, ic->ic_awdl_bssid_valid ? ic->ic_awdl_bssid : kAWDLBssid, 6);
            uint16_t sc = (uint16_t)(fTxSeq++ << 4);
            wh->i_seq[0] = sc & 0xff;
            wh->i_seq[1] = sc >> 8;
            uint8_t *l = frame + hdr;
            l[0] = 0xaa; l[1] = 0xaa; l[2] = 0x03;
            l[3] = 0x00; l[4] = 0x17; l[5] = 0xf2;
            l[6] = 0x08; l[7] = 0x00;
            uint8_t *a = l + llc;
            a[0] = 0x03; a[1] = 0x04;
            a[2] = fDataSeq & 0xff; a[3] = fDataSeq >> 8;
            a[6] = eh[12]; a[7] = eh[13];
            mbuf_copydata(m, 14, plen - 14, a + ad);
            int err = ic->ic_awdl_tx(ic, fTxChan, frame, hdr + llc + ad + plen - 14);
            if (err == EAGAIN || err == EBUSY || err == ENOBUFS) {
                fDataTxDeferred++;
                return;             // keep the original Ethernet packet for the next dwell
            }
            if (err) {
                fDataTxErr++;
            } else {
                fDataTx++;
                fDataSeq++;
                // if_output counts the bytes; packets come from the driver stats.
                if (fController && fController->fAWDLBsdIf && fController->fAWDLBsdIf->getIfnet())
                    ifnet_stat_increment_out(fController->fAWDLBsdIf->getIfnet(), 1, 0, 0);
            }
        }
        mbuf_freem(m);
        fDataQ[fDataQHead] = nullptr;
        fDataQHead = (fDataQHead + 1) % kDataQueueLen;
        fDataQCount--;
    }
}

void AirportItlwmAWDLInterface::
enqueueLowLatencyData(mbuf_t m)
{
    if (!m)
        return;
    if (fLowLatencyDataQCount >= kLowLatencyDataQueueLen) {
        auto *llw = fController ? fController->fLLWBsdIf : nullptr;
        ifnet_t llwIfp = llw ? llw->getIfnet() : nullptr;
        if (llwIfp)
            ifnet_stat_increment_out(llwIfp, 0, 1, 0);
        mbuf_freem(m);
        return;
    }
    fLowLatencyDataQ[(fLowLatencyDataQHead + fLowLatencyDataQCount) % kLowLatencyDataQueueLen] = m;
    fLowLatencyDataQCount++;
}

void AirportItlwmAWDLInterface::
clearLowLatencyData()
{
    while (fLowLatencyDataQCount) {
        mbuf_freem(fLowLatencyDataQ[fLowLatencyDataQHead]);
        fLowLatencyDataQ[fLowLatencyDataQHead] = nullptr;
        fLowLatencyDataQHead = (fLowLatencyDataQHead + 1) % kLowLatencyDataQueueLen;
        fLowLatencyDataQCount--;
    }
}

void AirportItlwmAWDLInterface::
flushLowLatencyData()
{
    ieee80211com *ic = fController ? fController->fHalService->get80211Controller() : nullptr;
    if (!ic || !ic->ic_awdl_llw_tx || !ic->ic_awdl_listener_chan)
        return;
    ifnet_t llwIfp = fController && fController->fLLWBsdIf ?
        fController->fLLWBsdIf->getIfnet() : nullptr;
    static uint8_t frame[2048];
    const size_t hdr = sizeof(struct ieee80211_frame);
    const size_t qos = ic->ic_awdl_llw_qos ? 2 : 0;
    const size_t llc = 8;
    static const uint8_t snap[6] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };

    while (fLowLatencyDataQCount) {
        mbuf_t m = fLowLatencyDataQ[fLowLatencyDataQHead];
        size_t plen = mbuf_pkthdr_len(m);
        if (plen < 14 || hdr + qos + llc + plen - 14 > sizeof(frame)) {
            if (llwIfp)
                ifnet_stat_increment_out(llwIfp, 0, 1, 0);
        } else {
            uint8_t eh[14];
            mbuf_copydata(m, 0, sizeof(eh), eh);
            memset(frame, 0, hdr + qos + llc);
            struct ieee80211_frame *wh = (struct ieee80211_frame *)frame;
            wh->i_fc[0] = IEEE80211_FC0_VERSION_0 | IEEE80211_FC0_TYPE_DATA;
            if (qos)
                wh->i_fc[0] |= IEEE80211_FC0_SUBTYPE_QOS;
            memcpy(wh->i_addr1, eh, 6);
            memcpy(wh->i_addr2, &fMac, 6);
            memcpy(wh->i_addr3,
                ic->ic_awdl_bssid_valid ? ic->ic_awdl_bssid : kAWDLBssid, 6);
            const uint16_t sc = (uint16_t)((fLowLatencySeq & 0x0fff) << 4);
            wh->i_seq[0] = sc & 0xff;
            wh->i_seq[1] = sc >> 8;
            if (qos)
                frame[hdr] = 6;
            uint8_t *l = frame + hdr + qos;
            memcpy(l, snap, sizeof(snap));
            l[6] = eh[12];
            l[7] = eh[13];
            mbuf_copydata(m, 14, plen - 14, l + llc);
            int err = ic->ic_awdl_llw_tx(ic, ic->ic_awdl_listener_chan, frame,
                hdr + qos + llc + plen - 14);
            if (err == EAGAIN || err == EBUSY || err == ENOBUFS)
                return;
            if (err) {
                if (llwIfp)
                    ifnet_stat_increment_out(llwIfp, 0, 1, 0);
            } else {
                fLowLatencySeq++;
                if (llwIfp)
                    ifnet_stat_increment_out(llwIfp, 1, 0, 0);
            }
        }
        mbuf_freem(m);
        fLowLatencyDataQ[fLowLatencyDataQHead] = nullptr;
        fLowLatencyDataQHead = (fLowLatencyDataQHead + 1) % kLowLatencyDataQueueLen;
        fLowLatencyDataQCount--;
    }
}

void AirportItlwmAWDLInterface::
publishDataState()
{
    setProperty("AWDLDataRx", fDataRx, 64);
    setProperty("AWDLDataRxBad", fDataRxBad, 64);
    setProperty("AWDLDataRxAmsdu", fDataRxAmsdu, 64);
    setProperty("AWDLDataRxNoBuf", fDataRxNoBuf, 64);
    setProperty("AWDLDataRxLastLen", fDataRxLastLen, 32);
    setProperty("AWDLDataRxLastType", fDataRxLastType, 32);
    setProperty("AWDLDataTx", fDataTx, 64);
    setProperty("AWDLDataTxQueued", fDataTxQueued, 64);
    setProperty("AWDLDataTxDropFull", fDataTxDropFull, 64);
    setProperty("AWDLDataTxDeferred", fDataTxDeferred, 64);
    setProperty("AWDLDataTxWithinDwell", fDataTxWithinDwell, 64);
    setProperty("AWDLDataTxErr", fDataTxErr, 64);
    setProperty("AWDLDataTxPending", fDataQCount, 32);
    setProperty("AWDLDataTxNotCommon", fDataTxNotCommon, 64);
    setProperty("AWDLListenOwn", fListenOwn, 64);
    setProperty("AWDLListenPeerSlots", fListenPeerSlots, 64);
}

void AirportItlwmAWDLInterface::
publishTxState()
{
    setProperty("AWDLTxPSF", fTxPSF, 64);
    setProperty("AWDLTxMI", fTxMI, 64);
    setProperty("AWDLTxRefused", fTxRefused, 64);
    setProperty("AWDLTxSkipMode", fTxSkipMode, 64);
    setProperty("AWDLTxLastErr", fTxLastErr, 32);
    setProperty("AWDLTxLastLen", fTxLastLen, 32);
    ieee80211com *ic = fController ? fController->fHalService->get80211Controller() : nullptr;
    if (!ic)
        return;
    setProperty("AWDLTxFwSent", ic->ic_awdl_tx_sent, 32);
    setProperty("AWDLTxFwBusy", ic->ic_awdl_tx_busy, 32);
    setProperty("AWDLTxFwErr", ic->ic_awdl_tx_err, 32);
    setProperty("AWDLTxFwOk", ic->ic_awdl_tx_ok, 32);
    setProperty("AWDLTxFwFail", ic->ic_awdl_tx_fail, 32);
    setProperty("AWDLTxFwLastStatus", ic->ic_awdl_tx_last_status, 32);
    setProperty("AWDLRxUcAction", ic->ic_awdl_rx_uc_action, 32);
    setProperty("AWDLRxUcData", ic->ic_awdl_rx_uc_data, 32);
    setProperty("AWDLRxUcOther", ic->ic_awdl_rx_uc_other, 32);
    setProperty("AWDLRxUcLastChannel", ic->ic_awdl_rx_uc_last_chan, 32);
    setProperty("AWDLRxUcChan6", ic->ic_awdl_rx_uc_by_chan[0], 32);
    setProperty("AWDLRxUcChan44", ic->ic_awdl_rx_uc_by_chan[1], 32);
    setProperty("AWDLRxUcChan149", ic->ic_awdl_rx_uc_by_chan[2], 32);
    setProperty("AWDLRxUcChanOther", ic->ic_awdl_rx_uc_by_chan[3], 32);
    setProperty("AWDLBaAccept", ic->ic_awdl_ba_accept ? true : false);
    {
        char ls[96];
        snprintf(ls, sizeof(ls), "chan=%u tries=%u failStage=%u failCode=%d", ic->ic_awdl_listener_chan,
            ic->ic_awdl_listener_tries, ic->ic_awdl_listener_stage, ic->ic_awdl_listener_code);
        setProperty("AWDLListener", ls);
    }
    setProperty("AWDLBaReq", ic->ic_awdl_ba_req, 32);
    setProperty("AWDLBaRespSent", ic->ic_awdl_ba_resp_sent, 32);
    setProperty("AWDLBaRespErr", ic->ic_awdl_ba_resp_err, 32);
    setProperty("AWDLFwUcMgmt", ic->ic_awdl_fw_uc_mgmt, 32);
    setProperty("AWDLFwUcData", ic->ic_awdl_fw_uc_data, 32);
    setProperty("AWDLFwUcBadCrc", ic->ic_awdl_fw_uc_badcrc, 32);
    {
        char mc[160];
        snprintf(mc, sizeof(mc), "v0:%u/%u v1:%u/%u v2:%u/%u v3:%u/%u",
            ic->ic_awdl_fw_mc_awdl[0], ic->ic_awdl_fw_mc_mgmt[0], ic->ic_awdl_fw_mc_awdl[1], ic->ic_awdl_fw_mc_mgmt[1],
            ic->ic_awdl_fw_mc_awdl[2], ic->ic_awdl_fw_mc_mgmt[2], ic->ic_awdl_fw_mc_awdl[3], ic->ic_awdl_fw_mc_mgmt[3]);
        setProperty("AWDLFwMcAwdlOverMgmt", mc);
        snprintf(mc, sizeof(mc), "v1:%u v2:%u v3:%u", ic->ic_awdl_mc_variant_roc[1], ic->ic_awdl_mc_variant_roc[2], ic->ic_awdl_mc_variant_roc[3]);
        setProperty("AWDLMcVariantListens", mc);
    }
    setProperty("AWDLFwUcDropDecrypt", ic->ic_awdl_fw_uc_drop_decrypt, 32);
    setProperty("AWDLFwUcLastChannel", ic->ic_awdl_fw_uc_last_chan, 32);
    setProperty("AWDLFwUcLastFc", ic->ic_awdl_fw_uc_last_fc, 32);
    setProperty("AWDLFwForeignData", ic->ic_awdl_fw_foreign_data, 32);
    setProperty("AWDLPromisc", ic->ic_awdl_want_data ? true : false);
    setProperty("AWDLPromiscChanges", ic->ic_awdl_promisc_changes, 32);
    // Which net80211 drop path feeds the station's input errors.
    char st[256];
    snprintf(st, sizeof(st), "ccmp_dec %u unenc %u wepfail %u wrongdir %u wrongbss %u tooshort %u notassoc %u dup %u decap %u ctl %u",
             ic->ic_stats.is_ccmp_dec_errs, ic->ic_stats.is_rx_unencrypted, ic->ic_stats.is_rx_wepfail,
             ic->ic_stats.is_rx_wrongdir, ic->ic_stats.is_rx_wrongbss, ic->ic_stats.is_rx_tooshort,
             ic->ic_stats.is_rx_notassoc, ic->ic_stats.is_rx_dup, ic->ic_stats.is_rx_decap, ic->ic_stats.is_rx_ctl);
    setProperty("NetRxDropStats", st);
    setProperty("AWDLP2PAddrChanges", ic->ic_awdl_p2p_addr_changes, 32);
    if (ic->ic_awdl_rx_uc_action + ic->ic_awdl_rx_uc_data + ic->ic_awdl_rx_uc_other) {
        char src[24];
        snprintf(src, sizeof(src), "%s", ether_sprintf(ic->ic_awdl_rx_uc_last_src));
        setProperty("AWDLRxUcLastSrc", src);
    }
    char me[24];
    snprintf(me, sizeof(me), "%s", ether_sprintf(ic->ic_awdl_addr));
    setProperty("AWDLRadioAddr", me);
    if (ic->ic_awdl_bssid_valid) {
        char bs[24];
        snprintf(bs, sizeof(bs), "%s", ether_sprintf(ic->ic_awdl_bssid));
        setProperty("AWDLRadioBssid", bs);
    }
}

void AirportItlwmAWDLInterface::
publishSyncState()
{
    char buf[24];
    snprintf(buf, sizeof(buf), "%s", ether_sprintf(fSyncMaster.octet));
    setProperty("AWDLSyncMaster", buf);
    setProperty("AWDLSyncMode", fSyncMode, 32);
    setProperty("AWDLAWRunning", fAWRunning);
    setProperty("AWDLAWStarts", fAWStarts, 64);
    setProperty("AWDLAWSeq", fLastSeq, 32);
    setProperty("AWDLSyncUpdates", fSyncUpdates, 64);
    setProperty("AWDLSyncErrUs", (uint64_t)fLastSyncErrUs, 32);
    setProperty("AWDLPresenceMode", presenceMode(), 32);
}

#pragma mark - creation, mirrors AppleBCMWLANCore::createProxInterface

IOReturn AirportItlwmSkywalkInterface::
setVIRTUAL_IF_CREATE(apple80211_virt_if_create_data *data)
{
    AirportItlwm *controller = OSDynamicCast(AirportItlwm, getController());
    if (!controller || !data)
        return kIOReturnBadArgument;
    return controller->createAWDLInterface(data);
}

IOReturn AirportItlwm::
createAWDLInterface(apple80211_virt_if_create_data *data)
{
    XYLog("AWDL %s: role %u enabled %d\n", __FUNCTION__, data->role, fAWDLEnabled);
    if (!fAWDLEnabled || data->role != kAirportItlwmRoleAWDL)
        return kIOReturnUnsupported;
    if (fAWDLIf) {
        strlcpy((char *)data->bsd_name, fAWDLIf->getBSDName() ?: "awdl0", sizeof(data->bsd_name));
        return kIOReturnSuccess;
    }

    ether_addr mac;
    memcpy(mac.octet, data->mac, sizeof(mac.octet));
    AirportItlwmAWDLInterface *awdl = AirportItlwmAWDLInterface::withController(this, &mac);
    if (!awdl)
        return kIOReturnNoMemory;
    if (!awdl->attach(this)) {
        XYLogE("AWDL %s: attach failed\n", __FUNCTION__);
        awdl->release();
        return kIOReturnError;
    }

    // Same registration sequence the station interface uses in start().
    IOSkywalkEthernetInterface::RegistrationInfo info;
    memset(&info, 0, sizeof(info));
    if (!awdl->initRegistrationInfo(&info, 1, sizeof(info))) {
        XYLogE("AWDL %s: initRegistrationInfo failed\n", __FUNCTION__);
        awdl->detach(this);
        awdl->release();
        return kIOReturnError;
    }
    awdl->mExpansionData->fRegistrationInfo = (IOSkywalkNetworkInterface::RegistrationInfo *)IOMalloc(sizeof(IOSkywalkNetworkInterface::RegistrationInfo));
    awdl->mExpansionData2->fRegistrationInfo = (IOSkywalkEthernetInterface::RegistrationInfo *)IOMalloc(sizeof(IOSkywalkEthernetInterface::RegistrationInfo));
    if (!awdl->mExpansionData->fRegistrationInfo || !awdl->mExpansionData2->fRegistrationInfo) {
        awdl->detach(this);
        awdl->release();
        return kIOReturnNoMemory;
    }
    memcpy(awdl->mExpansionData->fRegistrationInfo, &info, sizeof(info));
    fAWDLNetInfo = awdl->mExpansionData->fRegistrationInfo;
    fAWDLEthInfo = awdl->mExpansionData2->fRegistrationInfo;
    memcpy(awdl->mExpansionData2->fRegistrationInfo, &info, sizeof(info));
    awdl->deferBSDAttach(true);
    if (!awdl->start(this)) {
        XYLogE("AWDL %s: start failed\n", __FUNCTION__);
        awdl->detach(this);
        awdl->release();
        return kIOReturnError;
    }
    fAWDLIf = awdl;

    // The BSD ifnet comes from a dedicated IOEthernetInterface, as for the station.
    IONetworkInterface *bsd = nullptr;
    fAWDLMac = mac;
    fCreatingAWDL = true;
    bool ok = IONetworkController::attachInterface(&bsd, false);
    fCreatingAWDL = false;
    if (!ok || !bsd) {
        XYLogE("AWDL %s: BSD interface attach failed\n", __FUNCTION__);
        destroyAWDLInterface();
        return kIOReturnError;
    }
    fAWDLBsdIf = OSDynamicCast(AirportItlwmAWDLEthernetInterface, bsd);
    bsd->registerService();

    // The unit is assigned asynchronously by the network stack; awdl0 is the only
    // awdl interface on this machine, so report the name it will get.
    strlcpy((char *)data->bsd_name, "awdl0", sizeof(data->bsd_name));
    XYLog("AWDL %s: created %s\n", __FUNCTION__, data->bsd_name);
    return kIOReturnSuccess;
}

IOReturn AirportItlwm::
setAWDLLinkGated(OSObject *owner, void *arg0, void *, void *, void *)
{
    AirportItlwm *that = OSDynamicCast(AirportItlwm, owner);
    if (!that || !that->fAWDLIf)
        return kIOReturnNotReady;
    // Same call as AppleBCMWLANProximityInterface::bringupLink.
    that->fAWDLIf->setLinkState(arg0 ? kIO80211NetworkLinkUp : kIO80211NetworkLinkDown, 1);
    // The IOKit path that normally sets IFF_RUNNING is bypassed in performCommand.
    // Without it every send on awdl0 fails with ENETDOWN before reaching the driver.
    if (awDataAllowed() && that->fAWDLBsdIf && that->fAWDLBsdIf->getIfnet())
        ifnet_set_flags(that->fAWDLBsdIf->getIfnet(), arg0 ? IFF_RUNNING : 0, IFF_RUNNING);
    return kIOReturnSuccess;
}

// XYLogE is kprintf, which the unified log does not carry: a tester's capture never shows it. The outcome of the llw0 create
// goes to the I/O Registry (AWDLLlwStatus) and, for a failure, to IOLog as well.
#define AWDL_LLW_FAILED(step) do { \
    XYLogE("AWDL llw0: create failed at step " #step "\n"); \
    IOLog("itlwm: AWDL llw0: create failed at step " #step "\n"); \
    setProperty("AWDLLlwStatus", "failed at step " #step); \
} while (0)

IOReturn AirportItlwm::
createLowLatencyInterface(char *bsdName)
{
    if (!bsdName || !fAWDLEnabled || !fAWDLIf)
        return kIOReturnNotReady;
    if (!fHalService->get80211Controller()->ic_awdl_llw_tx)
        return kIOReturnUnsupported;
    if (fLLWBsdIf)
        return completeLowLatencyInterface(bsdName);
    if (fLLWIf)
        return kIOReturnBusy;

    // IO80211SkywalkInterface::getSelfMacAddr() returns the six bytes by value (in RAX), not a pointer to them. The
    // header declared a pointer, so this copy dereferenced the address itself: a general protection fault in
    // wifip2pd's request on every boot with -itlawdl, on every card (the RSI of all seven reported panics, read as a
    // little-endian value, is a locally administered MAC with two zero bytes above it). The address is ours anyway.
    fAWDLIf->copyAddress(fLLWMac);
    static const uint8_t zeroMac[sizeof(fLLWMac)] = {};
    if (!memcmp(&fLLWMac, zeroMac, sizeof(fLLWMac)))
        return kIOReturnNotReady;
    ether_addr *parentMac = &fLLWMac;

    auto *llw = new AirportItlwmLowLatencyInterface;
    if (!llw)
        return kIOReturnNoMemory;
    llw->setParentAddress(*parentMac);
    if (!llw->init(this)) {
        llw->release();
        return kIOReturnError;
    }
    fLLWIf = llw;
    fLLWAttached = false;
    fLLWStarted = false;
    llw->setInterfaceRole(kAirportItlwmLowLatencyRole);
    llw->setInterfaceId(kAirportItlwmLowLatencyInterfaceId);
    awdlSetLowLatencyEnabled(llw, true);
    awdlSetParentInterface(llw, fAWDLIf);
    awdlSetCompanionLowLatencySkywalkInterface(fAWDLIf, llw);
    awdlSetCompanionInterfaceId(llw, awdlGetInterfaceId(fAWDLIf));
    if (!llw->attach(this)) {
        AWDL_LLW_FAILED(1);
        destroyLowLatencyInterface();
        return kIOReturnError;
    }
    fLLWAttached = true;

    IOSkywalkEthernetInterface::RegistrationInfo info;
    memset(&info, 0, sizeof(info));
    if (!llw->initRegistrationInfo(&info, 1, sizeof(info))) {
        AWDL_LLW_FAILED(2);
        destroyLowLatencyInterface();
        return kIOReturnError;
    }

    // Match Tahoe's role-2 RegistrationInfo fields for AppleBCMWLANLowLatencyInterface.
    auto put32 = [](uint8_t *dst, uint32_t value) { memcpy(dst, &value, sizeof(value)); };
    const uint64_t registrationFlags = 0x8000000080ULL;
    const uint16_t txHeadroom = 64;
    const uint32_t lowLatencyFlags = 4;
    const char *prefix = "llw";
    put32(info.bytes + 0x0c, 3);
    put32(info.bytes + 0x14, kAirportItlwmLowLatencyRole);
    memcpy(info.bytes + 0x24, &registrationFlags, sizeof(registrationFlags));
    memcpy(info.bytes + 0x30, &prefix, sizeof(prefix));
    put32(info.bytes + 0x38, 0);
    memcpy(info.bytes + 0x3c, &txHeadroom, sizeof(txHeadroom));
    put32(info.bytes + 0x40, 0);
    memcpy(info.bytes + 0x108, fLLWMac.octet, 4);
    memcpy(info.bytes + 0x10c, fLLWMac.octet + 4, 2);
    put32(info.bytes + 0x120, lowLatencyFlags);

    auto *networkInfo = (IOSkywalkNetworkInterface::RegistrationInfo *)IOMalloc(
        sizeof(IOSkywalkNetworkInterface::RegistrationInfo));
    auto *ethernetInfo = (IOSkywalkEthernetInterface::RegistrationInfo *)IOMalloc(
        sizeof(IOSkywalkEthernetInterface::RegistrationInfo));
    if (!networkInfo || !ethernetInfo) {
        if (networkInfo) IOFree(networkInfo, sizeof(*networkInfo));
        if (ethernetInfo) IOFree(ethernetInfo, sizeof(*ethernetInfo));
        AWDL_LLW_FAILED(3);
        destroyLowLatencyInterface();
        return kIOReturnNoMemory;
    }
    memcpy(networkInfo, &info, sizeof(info));
    memcpy(ethernetInfo, &info, sizeof(info));
    llw->mExpansionData->fRegistrationInfo = networkInfo;
    llw->mExpansionData2->fRegistrationInfo = ethernetInfo;
    fLLWNetInfo = networkInfo;
    fLLWEthInfo = ethernetInfo;
    llw->deferBSDAttach(true);
    if (!llw->start(this)) {
        AWDL_LLW_FAILED(4);
        destroyLowLatencyInterface();
        return kIOReturnError;
    }
    fLLWStarted = true;

    IONetworkInterface *bsd = nullptr;
    fCreatingLLW = true;
    bool ok = IONetworkController::attachInterface(&bsd, false);
    fCreatingLLW = false;
    if (!ok || !bsd) {
        if (bsd) {
            detachInterface(bsd, true);
            bsd->release();
        }
        AWDL_LLW_FAILED(5);
        destroyLowLatencyInterface();
        return kIOReturnError;
    }
    fLLWBsdIf = OSDynamicCast(AirportItlwmLowLatencyEthernetInterface, bsd);
    if (!fLLWBsdIf) {
        detachInterface(bsd, true);
        bsd->release();
        AWDL_LLW_FAILED(6);
        destroyLowLatencyInterface();
        return kIOReturnError;
    }
    setProperty("AWDLLlwStatus", "registering");
    bsd->registerService(kIOServiceAsynchronous);
    return completeLowLatencyInterface(bsdName);
}

IOReturn AirportItlwm::
completeLowLatencyInterface(char *bsdName)
{
    auto *bsd = fLLWBsdIf;
    bsd->retain();
    if (!bsd->bsdRegistrationComplete()) {
        // BSD attach needs this same gate for prepareBSDInterface().
        AbsoluteTime deadline;
        clock_interval_to_deadline(1000, kMillisecondScale, reinterpret_cast<uint64_t *>(&deadline));
        while (fLLWBsdIf == bsd && !bsd->bsdRegistrationComplete()) {
            if (_fCommandGate->commandSleep(bsd, deadline, THREAD_INTERRUPTIBLE) != THREAD_AWAKENED)
                break;
        }
    }
    const bool current = fLLWBsdIf == bsd;
    const bool complete = bsd->bsdRegistrationComplete();
    const IOReturn result = complete ? bsd->bsdRegistrationResult() : kIOReturnNotReady;
    bsd->release();
    if (!current)
        return kIOReturnNotReady;
    if (!complete)
        return kIOReturnNotReady;
    ifnet_t ifp = bsd->getIfnet();
    if (result != kIOReturnSuccess || !ifp) {
        AWDL_LLW_FAILED(7);
        destroyLowLatencyInterface();
        return result != kIOReturnSuccess ? result : kIOReturnError;
    }

    bool linkUp = fHalService->get80211Controller()->ic_state == IEEE80211_S_RUN;
    bsd->setLinkState(linkUp ? kIO80211NetworkLinkUp : kIO80211NetworkLinkDown);
    bool llwUp = linkUp && (ifnet_flags(ifp) & IFF_UP);
    fLLWIf->setRunningState(llwUp);
    fLLWIf->reportLinkStatus(llwUp ? 3 : 1, llwUp ? 0x80 : 0);
    snprintf(bsdName, 16, "%s%u", ifnet_name(ifp), ifnet_unit(ifp));
    XYLog("AWDL companion created %s role %u id %u\n", bsdName,
          kAirportItlwmLowLatencyRole, kAirportItlwmLowLatencyInterfaceId);
    setProperty("AWDLLlwStatus", "created");
    return kIOReturnSuccess;
}

IOReturn AirportItlwm::
destroyLowLatencyInterface()
{
    if (!fLLWIf && !fLLWBsdIf)
        return kIOReturnNotFound;
    if (fAWDLIf) {
        awdlSetCompanionLowLatencySkywalkInterface(fAWDLIf, nullptr);
        fAWDLIf->clearLowLatencyData();
    }
    if (fLLWIf)
        awdlSetParentInterface(fLLWIf, nullptr);
    if (fLLWBsdIf) {
        auto *bsd = fLLWBsdIf;
        fLLWBsdIf = nullptr;
        if (_fCommandGate)
            _fCommandGate->commandWakeup(bsd);
        detachInterface(bsd, true);
        bsd->release();
    }
    if (fLLWIf) {
        takeOutOwnRegistrationInfo(fLLWIf, fLLWIf, fLLWNetInfo, fLLWEthInfo);
        if (fLLWStarted)
            fLLWIf->stop(this);
        if (fLLWAttached)
            fLLWIf->detach(this);
        fLLWIf->release();
        fLLWIf = nullptr;
    }
    freeOwnRegistrationInfo(fLLWNetInfo, fLLWEthInfo);
    fLLWStarted = false;
    fLLWAttached = false;
    memset(&fLLWMac, 0, sizeof(fLLWMac));
    return kIOReturnSuccess;
}

void AirportItlwm::
destroyAWDLInterface()
{
    destroyLowLatencyInterface();
    if (fAWDLBsdIf) {
        XYLog("AWDL %s: dropped %llu packets\n", __FUNCTION__, fAWDLBsdIf->droppedPackets());
        detachInterface(fAWDLBsdIf, true);
        fAWDLBsdIf->release();
        fAWDLBsdIf = nullptr;
    }
    if (fAWDLIf) {
        fAWDLIf->teardownAW();
        takeOutOwnRegistrationInfo(fAWDLIf, fAWDLIf, fAWDLNetInfo, fAWDLEthInfo);
        fAWDLIf->stop(this);
        fAWDLIf->detach(this);
        fAWDLIf->release();
        fAWDLIf = nullptr;
    }
    freeOwnRegistrationInfo(fAWDLNetInfo, fAWDLEthInfo);
}

// Log a setter that is accepted but not acted on yet.
#define AWDL_SET_STUB(NAME, TYPE)                       \
IOReturn AirportItlwmAWDLInterface::set##NAME(TYPE *d)  \
{                                                       \
    fCallCount++;                                       \
    AWDL_LOG("%p", d);                                  \
    return kIOReturnSuccess;                            \
}

// Getter without a known layout: tell the caller we have nothing.
#define AWDL_GET_STUB(NAME, TYPE)                       \
IOReturn AirportItlwmAWDLInterface::get##NAME(TYPE *d)  \
{                                                       \
    AWDL_LOG("unsupported");                            \
    return kIOReturnUnsupported;                        \
}

AirportItlwmAWDLInterface *AirportItlwmAWDLInterface::
withController(AirportItlwm *controller, ether_addr *mac)
{
    AirportItlwmAWDLInterface *ifp = new AirportItlwmAWDLInterface;
    if (!ifp)
        return nullptr;
    if (!ifp->init(controller, mac, kAirportItlwmRoleAWDL, "awdl")) {
        ifp->release();
        return nullptr;
    }
    ifp->fController = controller;
    // Runs on the controller work loop, like the rx path and the ioctls, so the
    // timeline state needs no extra locking.
    if (!awTimelineAllowed())
        return ifp;
    ifp->fAWTimer = IOTimerEventSource::timerEventSource(kIOTimerEventSourceOptionsPriorityHigh,
                                                         ifp, &AirportItlwmAWDLInterface::awTimerFired);
    if (!ifp->fAWTimer || !controller->getWorkLoop() ||
        controller->getWorkLoop()->addEventSource(ifp->fAWTimer) != kIOReturnSuccess) {
        AWDL_LOG("AW timer setup failed");
        OSSafeReleaseNULL(ifp->fAWTimer);
    }
    ifp->fPeerCacheTimer = IOTimerEventSource::timerEventSource(kIOTimerEventSourceOptionsPriorityHigh,
                                                                ifp, &AirportItlwmAWDLInterface::peerCacheTimerFired);
    if (!ifp->fPeerCacheTimer || !controller->getWorkLoop() ||
        controller->getWorkLoop()->addEventSource(ifp->fPeerCacheTimer) != kIOReturnSuccess) {
        AWDL_LOG("peer cache timer setup failed");
        OSSafeReleaseNULL(ifp->fPeerCacheTimer);
    }
    if (awUmiAckAllowed()) {
        ifp->fUmiAckTimer = IOTimerEventSource::timerEventSource(kIOTimerEventSourceOptionsPriorityHigh,
                                                                 ifp, &AirportItlwmAWDLInterface::umiAckTimerFired);
        if (!ifp->fUmiAckTimer || !controller->getWorkLoop() ||
            controller->getWorkLoop()->addEventSource(ifp->fUmiAckTimer) != kIOReturnSuccess) {
            AWDL_LOG("umi ack timer setup failed");
            OSSafeReleaseNULL(ifp->fUmiAckTimer);
        }
    }
    if (awListenAllowed()) {
        ifp->fListenTimer = IOTimerEventSource::timerEventSource(kIOTimerEventSourceOptionsPriorityHigh,
                                                                 ifp, &AirportItlwmAWDLInterface::listenTimerFired);
        if (!ifp->fListenTimer || !controller->getWorkLoop() ||
            controller->getWorkLoop()->addEventSource(ifp->fListenTimer) != kIOReturnSuccess) {
            AWDL_LOG("listen timer setup failed");
            OSSafeReleaseNULL(ifp->fListenTimer);
        }
    }
    if (awListenAllowed() && awTxAllowed()) {
        ifp->fTxTimer = IOTimerEventSource::timerEventSource(kIOTimerEventSourceOptionsPriorityHigh,
                                                             ifp, &AirportItlwmAWDLInterface::txTimerFired);
        if (!ifp->fTxTimer || !controller->getWorkLoop() ||
            controller->getWorkLoop()->addEventSource(ifp->fTxTimer) != kIOReturnSuccess) {
            AWDL_LOG("tx timer setup failed");
            OSSafeReleaseNULL(ifp->fTxTimer);
        }
    }
    return ifp;
}

bool AirportItlwmAWDLInterface::
init(IO80211Controller *controller, ether_addr *mac, unsigned int role, char const *prefix)
{
    if (!IO80211AWDLProtocol::init(controller, mac, role, prefix)) {
        AWDL_LOG("super::init failed");
        return false;
    }
    fController = nullptr;
    memcpy(&fMac, mac, sizeof(fMac));
    memset(&fSyncParams, 0, sizeof(fSyncParams));
    fSyncEnabled = false;
    fEncryptionType = APPLE80211_CIPHER_NONE;
    fSyncState = 0;
    fAwdlTrafficMask = 0;
    fCallCount = 0;
    fRxAF = 0;
    fRxAFDropped = 0;
    fAWTimer = nullptr;
    fPeerCacheTimer = nullptr;
    fUmiAckTimer = nullptr;
    fUmiAckCount = fUmiAckPosted = 0;
    fPeerCacheDoneCount = fPeerCacheCompleted = fPeerCacheDropped = 0;
    fAWRunning = false;
    fRefUs = 0;
    fRefSeq = 0;
    fLastValid = false;
    fLastStartUs = 0;
    fLastSeq = 0;
    fAWStarts = 0;
    fSyncUpdates = 0;
    fLastSyncErrUs = 0;
    memset(&fSyncMaster, 0, sizeof(fSyncMaster));
    fSyncMode = 0;
    fPresenceMode = 0;
    memset(fChanSeq, 0, sizeof(fChanSeq));
    fChanSeqValid = false;
    fInAW = false;
    fTsfOffset = 0;
    fTsfValid = false;
    fSyncOutliers = 0;
    fSyncOutliersTotal = 0;
    fListenSlot = 0;
    fListenStarted = 0;
    fListenBusy = 0;
    fListenSkippedHome = 0;
    fListenDwellTU = kListenDwellTU;
    fListenEveryNth = kListenEveryNthSlot;
    fListenTimer = nullptr;
    fListenAlign = 1;
    fListenAlignedActive = false;
    fListenNextChan = 0;
    fListenNextDwellTU = 0;
    fListenAligned = 0;
    fListenNoSlot = 0;
    fListenHunt = 0;
    fHuntStartUs = 0;
    fPeerValid = false;
    fPeerSeenUs = 0;
    fPeerRefUs = 0;
    fPeerRefSeq = 0;
    fPeerPm = 0;
    fPeerSlotUs = 0;
    fPeerChanValid = false;
    fPeerUpdates = 0;
    fPeerPredHit = 0;
    fPeerPredMiss = 0;
    fPeerLastErrUs = 0;
    fRxAFChan6 = 0;
    fRxAFChan44 = 0;
    fRxAFChan149 = 0;
    fRxAFChanOther = 0;
    fRxAFChan161 = 0;
    memset(fListenOnChan, 0, sizeof(fListenOnChan));
    fSyncTmplLen = 0;
    fSyncTmplSets = 0;
    fMITmplLen = 0;
    fOOBSets = 0;
    fOOBAutoSets = 0;
    fAfTxMode = 0;
    fTxTimer = nullptr;
    fTxChan = 0;
    fTxPending = 0;
    fTxMIOffsetUs = 0;
    fTxMIBitmap = 0;
    fTxSlotUs = 0;
    fTxDwellEndUs = 0;
    fTxSeq = 0;
    fMasterChannel = 0;
    fTxPSF = 0;
    fTxMI = 0;
    fTxRefused = 0;
    fTxSkipMode = 0;
    fTxLastErr = 0;
    fTxLastLen = 0;
    fDataRx = 0;
    fDataRxBad = 0;
    fDataRxAmsdu = 0;
    fDataRxNoBuf = 0;
    fDataRxLastLen = 0;
    fDataRxLastType = 0;
    memset(fDataQ, 0, sizeof(fDataQ));
    fDataQHead = 0;
    fDataQCount = 0;
    memset(fLowLatencyDataQ, 0, sizeof(fLowLatencyDataQ));
    fLowLatencyDataQHead = 0;
    fLowLatencyDataQCount = 0;
    fLowLatencySeq = 0;
    fDataSeq = 0;
    fDataTx = 0;
    fDataTxQueued = 0;
    fDataTxDropFull = 0;
    fDataTxDeferred = 0;
    fDataTxWithinDwell = 0;
    fDataTxErr = 0;
    fListenOwn = 0;
    fListenPeerSlots = 0;
    memset(fRxGap, 0, sizeof(fRxGap));
    fBssidSets = 0;
    fPeerCacheOps = 0;
    fDataTxNotCommon = 0;
    AWDL_LOG("role %u prefix %s mac %s", role, prefix, ether_sprintf(mac->octet));
    return true;
}

void AirportItlwmAWDLInterface::
free()
{
    AWDL_LOG("calls %llu", fCallCount);
    teardownAW();
    IO80211AWDLProtocol::free();
}

IOReturn AirportItlwmAWDLInterface::
setMacAddress(ether_addr &mac)
{
    memcpy(&fMac, &mac, sizeof(fMac));
    AWDL_LOG("%s", ether_sprintf(mac.octet));
    publishAWDLAddress();
    return kIOReturnSuccess;
}

// Give the radio awdl0's address (P2P device MAC, unicast rx accounting).
void AirportItlwmAWDLInterface::
publishAWDLAddress()
{
    if (!awTxAllowed() || !fController)
        return;
    ieee80211com *ic = fController->fHalService->get80211Controller();
    if (!ic)
        return;
    if (ic->ic_awdl_addr_gen && !memcmp(ic->ic_awdl_addr, &fMac, sizeof(fMac)))
        return;
    memcpy(ic->ic_awdl_addr, &fMac, sizeof(fMac));
    ic->ic_awdl_addr_gen++;
}

#pragma mark - getters with cached state

IOReturn AirportItlwmAWDLInterface::
getSSID(apple80211_ssid_data *d)
{
    memset(d, 0, sizeof(*d));
    d->version = APPLE80211_VERSION;
    return kIOReturnSuccess;
}

IOReturn AirportItlwmAWDLInterface::
getCHANNEL(apple80211_channel_data *d)
{
    const ieee80211com *ic = fController ? fController->fHalService->get80211Controller() : nullptr;
    const uint8_t chan = ic && ic->ic_awdl_listener_chan ? ic->ic_awdl_listener_chan : 6;
    memset(d, 0, sizeof(*d));
    d->version = APPLE80211_VERSION;
    d->channel.version = APPLE80211_VERSION;
    d->channel.channel = chan;
    d->channel.flags = (chan <= 14 ? APPLE80211_C_FLAG_2GHZ : APPLE80211_C_FLAG_5GHZ) |
        APPLE80211_C_FLAG_20MHZ;
    return kIOReturnSuccess;
}

IOReturn AirportItlwmAWDLInterface::
getBSSID(apple80211_bssid_data *d)
{
    memset(d, 0, sizeof(*d));
    d->version = APPLE80211_VERSION;
    memcpy(&d->bssid, &fMac, sizeof(fMac));
    return kIOReturnSuccess;
}

IOReturn AirportItlwmAWDLInterface::
getSTATE(apple80211_state_data *d)
{
    memset(d, 0, sizeof(*d));
    d->version = APPLE80211_VERSION;
    d->state = APPLE80211_S_INIT;
    return kIOReturnSuccess;
}

IOReturn AirportItlwmAWDLInterface::
getAWDL_SYNC_PARAMS(apple80211_awdl_sync_params *d)
{
    memcpy(d, &fSyncParams, sizeof(*d));
    return kIOReturnSuccess;
}

IOReturn AirportItlwmAWDLInterface::
getAWDL_SYNC_ENABLED(apple80211_awdl_sync_enabled *d)
{
    memset(d, 0, sizeof(*d));
    d->version = APPLE80211_VERSION;
    d->enabled = fSyncEnabled;
    return kIOReturnSuccess;
}

IOReturn AirportItlwmAWDLInterface::
getAWDL_SYNC_STATE(apple80211_awdl_sync_state *d)
{
    memset(d, 0, sizeof(*d));
    d->version = APPLE80211_VERSION;
    d->state = fSyncState;
    return kIOReturnSuccess;
}

IOReturn AirportItlwmAWDLInterface::
getPEER_CACHE_MAXIMUM_SIZE(apple80211_peer_cache_maximum_size *d)
{
    memset(d, 0, sizeof(*d));
    d->version = APPLE80211_VERSION;
    d->max_peers = 32;
    return kIOReturnSuccess;
}

#pragma mark - setters with cached state

IOReturn AirportItlwmAWDLInterface::
setAWDL_SYNC_PARAMS(apple80211_awdl_sync_params *d)
{
    fCallCount++;
    memcpy(&fSyncParams, d, sizeof(fSyncParams));
    if (fController) {
        auto ic = fController->fHalService->get80211Controller();
        ic->ic_awdl_sync_period_tu = d->synchronization_frame_period ?:
            IEEE80211_AWDL_DEFAULT_SYNC_PERIOD_TU;
    }
    AWDL_LOG("aw_len %u aw_period %u ext_len %u sync_frame_period %u",
             d->availability_window_length, d->availability_window_period,
             d->extension_length, d->synchronization_frame_period);
    return kIOReturnSuccess;
}

IOReturn AirportItlwmAWDLInterface::
setAWDL_SYNC_ENABLED(apple80211_awdl_sync_enabled *d)
{
    fCallCount++;
    fSyncEnabled = d->enabled != 0;
    AWDL_LOG("enabled %u", d->enabled);
    if (!awTimelineAllowed())
        return kIOReturnSuccess;
    setProperty("AWDLSyncEnabled", fSyncEnabled);
    if (fSyncEnabled)
        startAWTimeline();
    else
        stopAWTimeline();
    return kIOReturnSuccess;
}

IOReturn AirportItlwmAWDLInterface::
setAWDL_SYNC_STATE(apple80211_awdl_sync_state *d)
{
    fCallCount++;
    // IO80211AWDLPeerManager::programChipWithNewMaster: +4 mode, +8 the master
    // to follow (our own address when we are master), +16 op mode, +20 flags.
    const uint8_t *raw = (const uint8_t *)d;
    fSyncState = d->state;
    AWDL_LOG("state %u", d->state);
    if (!awTimelineAllowed())
        return kIOReturnSuccess;
    memcpy(&fSyncMaster, raw + 8, sizeof(fSyncMaster));
    fSyncMode = *(const uint32_t *)(raw + 16);
    AWDL_LOG("state %u master %s mode %u flags 0x%x", d->state, ether_sprintf(fSyncMaster.octet),
             fSyncMode, raw[20]);
    publishSyncState();
    return kIOReturnSuccess;
}

// +4 BSSID (Broadcom stores it at ivars +608 and creates its firmware AWDL
// interface with it, AppleBCMWLANProximityInterface::setAWDL_BSSID); all zero
// tears the interface down. +10: a second address, logged only.
IOReturn AirportItlwmAWDLInterface::
setAWDL_BSSID(apple80211_awdl_bssid *d)
{
    fCallCount++;
    AWDL_LOG("bssid %s", ether_sprintf(d->bssid));
    if (!awTimelineAllowed())
        return kIOReturnSuccess;
    char b[48];
    snprintf(b, sizeof(b), "%s", ether_sprintf(d->bssid));
    size_t n = strlen(b);
    snprintf(b + n, sizeof(b) - n, " %s", ether_sprintf(d->unk_mac));
    setProperty("AWDLBssid", b);
    fBssidSets++;
    setProperty("AWDLBssidSets", fBssidSets, 64);
    if (!awDataAllowed() || !fController)
        return kIOReturnSuccess;
    ieee80211com *ic = fController->fHalService->get80211Controller();
    if (!ic)
        return kIOReturnSuccess;
    static const uint8_t zero[6] = {};
    bool valid = memcmp(d->bssid, zero, 6) != 0;
    if (valid == (ic->ic_awdl_bssid_valid != 0) && (!valid || !memcmp(ic->ic_awdl_bssid, d->bssid, 6)))
        return kIOReturnSuccess;
    memcpy(ic->ic_awdl_bssid, d->bssid, 6);
    ic->ic_awdl_bssid_valid = valid;
    ic->ic_awdl_addr_gen++;     // the P2P MAC is re-programmed before the next dwell
    return kIOReturnSuccess;
}

// Broadcom: awdl_peer_op puts the peer (address, HT/VHT/HE caps, its channel
// sequence) into the firmware peer cache before unicast. Layout as read by
// AppleBCMWLANProximityInterface::setPEER_CACHE_CONTROL: +4 opcode, +8 MAC,
// +16 HT IE (45, 26, ...), +44 ampdu, +45 VHT present, +46 VHT IE, +60 HE,
// +68.. channel sequence.
IOReturn AirportItlwmAWDLInterface::
setPEER_CACHE_CONTROL(apple80211_peer_cache_control *d)
{
    fCallCount++;
    if (!awTimelineAllowed() || !d)
        return kIOReturnSuccess;
    const uint8_t *r = (const uint8_t *)d;
    uint32_t op = *(const uint32_t *)(r + 4);
    fPeerCacheOps++;
    char s[96];
    snprintf(s, sizeof(s), "op %u %s ht %u/%u vht %u he %u", op, ether_sprintf((uint8_t *)r + 8),
             r[16], r[17], r[45], r[60]);
    setProperty("AWDLPeerCacheLast", s);
    setProperty("AWDLPeerCacheOps", fPeerCacheOps, 64);
    if (_fCommandGate && fController && (op == 0 || op == 2 || op == 6))
        _fCommandGate->runAction([](OSObject *, void *owner, void *data, void *operation, void *) -> IOReturn {
            auto that = static_cast<AirportItlwmAWDLInterface *>(owner);
            ieee80211com *ic = that->fController->fHalService->get80211Controller();
            if (ic && ic->ic_awdl_peer_cache)
                ic->ic_awdl_peer_cache(ic, (uint32_t)(uintptr_t)operation, (const uint8_t *)data);
            // Answer it, after this handler has returned and the caller is asleep (see the member comment).
            if (that->fPeerCacheTimer) {
                if (that->fPeerCacheDoneCount < kPeerCacheQueueDepth) {
                    uint8_t *slot = that->fPeerCacheDone[that->fPeerCacheDoneCount++];
                    memcpy(slot, (const uint8_t *)data - 8, kPeerCacheBytes);
                    const uint32_t result = (uint32_t)(uintptr_t)operation == 2 ? 3 : 1;
                    memcpy(slot + 4, &result, sizeof(result));
                    that->fPeerCacheTimer->setTimeoutMS(10);
                } else {
                    that->fPeerCacheDropped++;
                }
            }
            return kIOReturnSuccess;
        }, this, (void *)(r + 8), (void *)(uintptr_t)op);
    return kIOReturnSuccess;
}

void AirportItlwmAWDLInterface::
umiAckTimerFired(OSObject *owner, IOTimerEventSource *)
{
    if (auto *self = OSDynamicCast(AirportItlwmAWDLInterface, owner))
        self->postUmiAcks();
}

void AirportItlwmAWDLInterface::
postUmiAcks()
{
#ifndef APPLE80211_M_AWDL_UNICAST_AF_STATUS
#define APPLE80211_M_AWDL_UNICAST_AF_STATUS 69
#endif
    uint8_t peers[kUmiAckDepth][6];
    const unsigned count = fUmiAckCount;
    memcpy(peers, fUmiAckPeers, sizeof(peers));
    fUmiAckCount = 0;
    for (unsigned i = 0; i < count; i++) {
        uint8_t status[32] = {};
        memcpy(status, peers[i], 6);            // status[6] = 0: acknowledged
        postMessage(APPLE80211_M_AWDL_UNICAST_AF_STATUS, status, sizeof(status), false);
        fUmiAckPosted++;
    }
    if (count)
        setProperty("AWDLUmiAckPosted", fUmiAckPosted, 32);
}

void AirportItlwmAWDLInterface::
peerCacheTimerFired(OSObject *owner, IOTimerEventSource *)
{
    if (auto *self = OSDynamicCast(AirportItlwmAWDLInterface, owner))
        self->postPeerCacheCompletions();
}

void AirportItlwmAWDLInterface::
postPeerCacheCompletions()
{
#ifndef APPLE80211_M_PEER_CACHE_CONTROL
#define APPLE80211_M_PEER_CACHE_CONTROL 51
#endif
    unsigned count = fPeerCacheDoneCount;
    fPeerCacheDoneCount = 0;
    for (unsigned i = 0; i < count; i++) {
        uint8_t record[kPeerCacheBytes];
        memcpy(record, fPeerCacheDone[i], sizeof(record));
        postMessage(APPLE80211_M_PEER_CACHE_CONTROL, record, sizeof(record), false);
        fPeerCacheCompleted++;
    }
    if (count) {
        setProperty("AWDLPeerCacheCompleted", fPeerCacheCompleted, 32);
        if (fPeerCacheDropped)
            setProperty("AWDLPeerCacheDropped", fPeerCacheDropped, 32);
    }
}

IOReturn AirportItlwmAWDLInterface::
setAWDL_PRESENCE_MODE(apple80211_awdl_presence_mode *d)
{
    fCallCount++;
    fPresenceMode = d->mode;
    AWDL_LOG("mode %u", d->mode);
    return kIOReturnSuccess;
}

#pragma mark - stage 3: transmit peer-manager PSF and MI payloads

// Payload pointers come from IO80211AWDLPeerManager's own buffers
// (updateSyncFrameTemplate: 512 bytes on its stack; buildMasterIndicationTemplate:
// 0x4b0 bytes in its ivars). Only copy from kernel addresses.
static bool awdlKernelPtr(const void *p)
{
    return (uintptr_t)p >= 0xffffff8000000000ULL;
}

// "type:len type:len ..." of a TLV buffer, to check which TLVs the system
// supplies and which the firmware is expected to add.
static void awdlTLVList(const uint8_t *b, size_t len, char *out, size_t outLen)
{
    size_t off = 0, n = 0;
    out[0] = 0;
    while (off + 3 <= len && n + 12 < outLen) {
        uint16_t tlen = b[off + 1] | (b[off + 2] << 8);
        n += snprintf(out + n, outLen - n, "%s%u:%u", n ? " " : "", b[off], tlen);
        off += 3 + tlen;
    }
    if (off != len && n + 12 < outLen)
        snprintf(out + n, outLen - n, " !%zu/%zu", off, len);
}

static void awdlHex(const uint8_t *b, size_t len, char *out, size_t outLen)
{
    size_t n = 0;
    out[0] = 0;
    for (size_t i = 0; i < len && n + 3 < outLen; i++)
        n += snprintf(out + n, outLen - n, "%02x", b[i]);
}

// ioctl 151, from IO80211AWDLPeerManager::updateSyncFrameTemplate: +4 length,
// +8 pointer. TLVs only (service params, election v2, data path state, ...);
// Broadcom hands it to the firmware ("awdl_payload"), which adds the AWDL
// header, the sync parameters and the channel sequence when it sends a PSF.
IOReturn AirportItlwmAWDLInterface::
setAWDL_SYNC_FRAME_TEMPLATE(apple80211_awdl_sync_frame_template *d)
{
    fCallCount++;
    const uint8_t *raw = (const uint8_t *)d;
    uint32_t len = *(const uint32_t *)(raw + 4);
    const uint8_t *p = *(const uint8_t * const *)(raw + 8);
    AWDL_LOG("version %u len %u", d->version, len);
    if (!awTimelineAllowed() || !len || len > sizeof(fSyncTmpl) || !awdlKernelPtr(p))
        return kIOReturnSuccess;
    memcpy(fSyncTmpl, p, len);
    fSyncTmplLen = (uint16_t)len;
    fSyncTmplSets++;
    char tlvs[160];
    awdlTLVList(fSyncTmpl, len, tlvs, sizeof(tlvs));
    setProperty("AWDLTxSyncTmplLen", len, 32);
    setProperty("AWDLTxSyncTmplSets", fSyncTmplSets, 64);
    setProperty("AWDLTxSyncTmplTLVs", tlvs);
    return kIOReturnSuccess;
}

// ioctl 149 (send now; also IO80211AWDLPeerManager::sendUnicastMI) and 225
// (auto, from updatePrimaryPayloadMI). Layout from those builders and from
// AppleBCMWLANProximityInterface::setAWDL_OOB_(AUTO_)REQUEST:
// +4 destination MAC (ff.. for broadcast MI), +12 tx offset in the AW (us,
// auto), +16 MI bitmap (auto), +20 u16 (6, or 14 for UMI on the dwell
// channel), +22 6 bytes from PeerManager ivars (inferred: our address or the
// AWDL BSSID), +40 payload length, +48 payload pointer. The payload is TLVs
// (election v2, data path state, AirDrop, arpa, service params/responses).
void AirportItlwmAWDLInterface::
recordOOB(const void *req, bool autoReq)
{
    const uint8_t *raw = (const uint8_t *)req;
    uint16_t len;
    const uint8_t *p;
    memcpy(&len, raw + 40, sizeof(len));
    memcpy(&p, raw + 48, sizeof(p));
    char hdr[48 * 2 + 1];
    awdlHex(raw, 48, hdr, sizeof(hdr));
    setProperty(autoReq ? "AWDLTxOOBAutoHdr" : "AWDLTxOOBHdr", hdr);
    if (autoReq) {
        uint32_t offsetUs, bitmap;
        memcpy(&offsetUs, raw + 12, sizeof(offsetUs));
        memcpy(&bitmap, raw + 16, sizeof(bitmap));
        fOOBAutoSets++;
        fMITmplLen = 0;
        fTxMIOffsetUs = offsetUs;
        fTxMIBitmap = bitmap;
        setProperty("AWDLTxOOBAutoSets", fOOBAutoSets, 64);
        setProperty("AWDLTxMIBitmap", bitmap, 32);
        setProperty("AWDLTxMIOffsetUs", offsetUs, 32);
    } else {
        fOOBSets++;
        setProperty("AWDLTxOOBSets", fOOBSets, 64);
    }
    if (!len || len > sizeof(fMITmpl) || !awdlKernelPtr(p))
        return;
    char tlvs[200];
    awdlTLVList(p, len, tlvs, sizeof(tlvs));
    setProperty(autoReq ? "AWDLTxOOBAutoTLVs" : "AWDLTxOOBTLVs", tlvs);
    setProperty(autoReq ? "AWDLTxOOBAutoLen" : "AWDLTxOOBLen", len, 32);
    if (autoReq) {
        memcpy(fMITmpl, p, len);
        fMITmplLen = len;
    }
}

IOReturn AirportItlwmAWDLInterface::
setAWDL_OOB_REQUEST(apple80211_awdl_oob_request *d)
{
    fCallCount++;
    if (!d || !fController || !_fCommandGate)
        return kIOReturnBadArgument;
    auto action = [](OSObject *, void *owner, void *arg, void *, void *) -> IOReturn {
        auto that = static_cast<AirportItlwmAWDLInterface *>(owner);
        const uint8_t *raw = static_cast<const uint8_t *>(arg);
        uint16_t len = *(const uint16_t *)(raw + 40);
        const uint8_t *payload = *(const uint8_t * const *)(raw + 48);
        if (len > sizeof(that->fMITmpl) || (len && !awdlKernelPtr(payload)))
            return kIOReturnBadArgument;
        if (awTimelineAllowed())
            that->recordOOB(raw, false);
        if (!awTxAllowed() || !that->fAWRunning || that->fAfTxMode == 3)
            return kIOReturnNotReady;
        ieee80211com *ic = that->fController->fHalService->get80211Controller();
        if (!ic || !ic->ic_awdl_tx || !ic->ic_awdl_listen_chan)
            return kIOReturnNotReady;
        // sendUnicastMI calls this after confirming a shared channel and AW.
        int err = that->sendAWDLFrame(true, (uint8_t)ic->ic_awdl_listen_chan,
            raw + 4, payload, len);
        if (err == EAGAIN || err == ENOBUFS)
            return kIOReturnBusy;
        if (!err && that->fUmiAckTimer && !(raw[4] & 1)) {
            // A unicast frame went out: owe macOS its status (the destination is the first address of the request).
            bool listed = false;
            for (unsigned i = 0; i < that->fUmiAckCount; i++)
                if (!memcmp(that->fUmiAckPeers[i], raw + 4, 6))
                    listed = true;
            if (!listed && that->fUmiAckCount < kUmiAckDepth)
                memcpy(that->fUmiAckPeers[that->fUmiAckCount++], raw + 4, 6);
            that->fUmiAckTimer->setTimeoutMS(150);
        }
        return err ? kIOReturnError : kIOReturnSuccess;
    };
    IOWorkLoop *workLoop = _fCommandGate->getWorkLoop();
    if (workLoop && workLoop->inGate())
        return action(_fCommandGate, this, d, nullptr, nullptr);
    return _fCommandGate->runAction(action, this, d);
}

IOReturn AirportItlwmAWDLInterface::
setAWDL_OOB_AUTO_REQUEST(apple80211_awdl_oob_request *d)
{
    fCallCount++;
    if (!d)
        return kIOReturnBadArgument;
    recordOOB(d, true);
    return kIOReturnSuccess;
}

// ioctl 208, IO80211AWDLPeerManager::setAFTXMode: +4 mode (3 = suppress, e.g.
// infra on 2.4 GHz; 0 = allow). Broadcom: iovar awdl_aftxmode.
// +4 channel number (AppleBCMWLANProximityInterface::setAWDL_MASTER_CHANNEL).
IOReturn AirportItlwmAWDLInterface::
setAWDL_MASTER_CHANNEL(apple80211_awdl_master_channel *d)
{
    fCallCount++;
    fMasterChannel = *(const uint32_t *)((const uint8_t *)d + 4);
    AWDL_LOG("channel %u", fMasterChannel);
    if (awTimelineAllowed())
        setProperty("AWDLMasterChannel", fMasterChannel, 32);
    return kIOReturnSuccess;
}

IOReturn AirportItlwmAWDLInterface::
setAWDL_AF_TX_MODE(apple80211_awdl_af_tx_mode *d)
{
    fCallCount++;
    fAfTxMode = *(const uint32_t *)((const uint8_t *)d + 4);
    AWDL_LOG("mode %u", fAfTxMode);
    if (awTimelineAllowed())
        setProperty("AWDLTxAfMode", fAfTxMode, 32);
    return kIOReturnSuccess;
}

IOReturn AirportItlwmAWDLInterface::
setAWDL_SYNCHRONIZATION_CHANNEL_SEQUENCE(apple80211_awdl_sync_channel_sequence *d)
{
    fCallCount++;
    if (!awTimelineAllowed())
        return kIOReturnSuccess;
    const uint8_t *raw = (const uint8_t *)d;
    if (raw[kChanSeqCount] < 1 || raw[kChanSeqCount] > kChanSeqMax)
        return kIOReturnBadArgument;
    memcpy(fChanSeq, raw, sizeof(fChanSeq));
    fChanSeqValid = true;
    char seq[kChanSeqMax * 4 + 1] = {};
    for (unsigned i = 0, n = 0; i < raw[kChanSeqCount] && n + 4 < sizeof(seq); i++) {
        const apple80211_channel *c = (const apple80211_channel *)(raw + kChanSeqEntries) + i;
        n += snprintf(seq + n, sizeof(seq) - n, " %u", c->channel);
    }
    AWDL_LOG("count %u enc %u step %u:%s", raw[kChanSeqCount], raw[6], raw[kChanSeqStep], seq);
    setProperty("AWDLChanSeq", seq);
    char hdr[32];
    snprintf(hdr, sizeof(hdr), "enc %u step %u dup %u fill %u", raw[6], raw[kChanSeqStep], raw[8], raw[9]);
    setProperty("AWDLChanSeqHdr", hdr);
    if (awDataAllowed() && awListenAllowed() && fAWRunning)
        scheduleAlignedListen(hostUs());
    return kIOReturnSuccess;
}

IOReturn AirportItlwmAWDLInterface::
getAWDL_SYNCHRONIZATION_CHANNEL_SEQUENCE(apple80211_awdl_sync_channel_sequence *d)
{
    if (!fChanSeqValid)
        return kIOReturnUnsupported;
    memcpy(d, fChanSeq, sizeof(fChanSeq));
    const ieee80211com *ic = fController ? fController->fHalService->get80211Controller() : nullptr;
    if (ic && ic->ic_awdl_listener_chan) {
        uint8_t *raw = (uint8_t *)d;
        if (raw[9] && raw[9] != 255)
            raw[9] = ic->ic_awdl_listener_chan;
        auto channels = (apple80211_channel *)(raw + kChanSeqEntries);
        for (unsigned i = 0; i < raw[kChanSeqCount]; ++i) {
            if (!channels[i].channel)
                continue;
            channels[i].channel = ic->ic_awdl_listener_chan;
            channels[i].flags = (channels[i].channel <= 14 ? APPLE80211_C_FLAG_2GHZ :
                APPLE80211_C_FLAG_5GHZ) | APPLE80211_C_FLAG_20MHZ;
        }
    }
    return kIOReturnSuccess;
}

IOReturn AirportItlwmAWDLInterface::
getAWDL_PRESENCE_MODE(apple80211_awdl_presence_mode *d)
{
    if (!awTimelineAllowed())
        return kIOReturnUnsupported;
    memset(d, 0, sizeof(*d));
    d->version = APPLE80211_VERSION;
    d->mode = presenceMode();
    return kIOReturnSuccess;
}

#pragma mark - getters without a known layout

AWDL_GET_STUB(AWDL_EXTENSION_STATE_MACHINE_PARAMETERS, apple80211_awdl_extension_state_machine_parameter)
AWDL_GET_STUB(AWDL_ELECTION_ID, apple80211_awdl_election_id)
AWDL_GET_STUB(AWDL_ELECTION_METRIC, apple80211_awdl_election_metric)
AWDL_GET_STUB(AWDL_MASTER_CHANNEL, apple80211_awdl_master_channel)
AWDL_GET_STUB(AWDL_ELECTION_RSSI_THRESHOLDS, apple80211_awdl_election_rssi_thresholds)
AWDL_GET_STUB(AWDL_ELECTION_PARAMS, apple80211_awdl_election_params)
AWDL_GET_STUB(AWDL_DEVICE_CAPABILITIES, apple80211_awdl_device_capabilities)
AWDL_GET_STUB(AWDL_SECONDARY_MASTER_CHANNEL, apple80211_awdl_secondary_master_channel)
AWDL_GET_STUB(AWDL_AF_TX_MODE, apple80211_awdl_af_tx_mode)
AWDL_GET_STUB(AWDL_MIN_RATE, apple80211_awdl_min_rate)
IOReturn AirportItlwmAWDLInterface::
getAWDL_ENCRYPTION_TYPE(apple80211_encrypt *d)
{
    if (!d)
        return kIOReturnBadArgument;
    reinterpret_cast<uint32_t *>(d)[1] = fEncryptionType;
    return kIOReturnSuccess;
}
AWDL_GET_STUB(AWDL_TXCAL_PERIOD, apple80211_awdl_txcal_period)
AWDL_GET_STUB(AWDL_DEV_STATS, apple80211_awdl_dev_stats)
AWDL_GET_STUB(AWDL_DFSP_CONFIG, apple80211_awdl_dfsp_config)
AWDL_GET_STUB(AWDL_CCA, apple80211_awdl_cca)
IOReturn AirportItlwmAWDLInterface::
getAWDL_AF_SECONDARY_PAYLOAD_CAPABILITIES(apple80211_awdl_cap *d)
{
    if (!d)
        return kIOReturnBadArgument;
    d->version = APPLE80211_VERSION;
    d->cap = 0;
    return kIOReturnSuccess;
}

IOReturn AirportItlwmAWDLInterface::
getLLW_PARAMS(apple80211_llw_params *d)
{
    fCallCount++;
    if (!d)
        return kIOReturnBadArgument;
    if (!_fCommandGate)
        return kIOReturnUnsupported;
    return _fCommandGate->runAction([](OSObject *, void *owner, void *arg, void *, void *) -> IOReturn {
        auto *that = static_cast<AirportItlwmAWDLInterface *>(owner);
        auto *params = reinterpret_cast<uint8_t *>(arg);
        params[0x0b] = (that->fAwdlTrafficMask & AirportItlwmAWDLInterface::kAwdlTrafficLLWParams) != 0;
        return kIOReturnSuccess;
    }, this, d);
}

#pragma mark - AWDL setters

AWDL_SET_STUB(SSID, apple80211_ssid_data)
AWDL_SET_STUB(CHANNEL, apple80211_channel_data)
AWDL_SET_STUB(AWDL_EXTENSION_STATE_MACHINE_PARAMETERS, apple80211_awdl_extension_state_machine_parameter)
AWDL_SET_STUB(AWDL_ELECTION_ID, apple80211_awdl_election_id)
AWDL_SET_STUB(AWDL_ELECTION_METRIC, apple80211_awdl_election_metric)
AWDL_SET_STUB(AWDL_ELECTION_RSSI_THRESHOLDS, apple80211_awdl_election_rssi_thresholds)
AWDL_SET_STUB(AWDL_ELECTION_PARAMS, apple80211_awdl_election_params)
IOReturn AirportItlwmAWDLInterface::
setAwdlTrafficReason(uint8_t reason, bool active)
{
    ieee80211com *ic = fController->fHalService->get80211Controller();
    if (!ic || !ic->ic_awdl_set_low_latency)
        return kIOReturnUnsupported;
    const uint8_t previous = fAwdlTrafficMask;
    const bool previousLowLatency = previous != 0;
    if (active)
        fAwdlTrafficMask |= reason;
    else
        fAwdlTrafficMask &= static_cast<uint8_t>(~reason);
    const bool lowLatency = fAwdlTrafficMask != 0;
    if (lowLatency == previousLowLatency)
        return kIOReturnSuccess;
    int err = ic->ic_awdl_set_low_latency(ic, lowLatency);
    if (err) {
        fAwdlTrafficMask = previous;
        return kIOReturnError;
    }
    setProperty("AWDLLowLatencyActive", lowLatency);
    return kIOReturnSuccess;
}

IOReturn AirportItlwmAWDLInterface::
setAWDL_PEER_TRAFFIC_REGISTRATION(apple80211_awdl_peer_traffic_registration *d)
{
    fCallCount++;
    if (!d || d->name_len == 0 || d->name_len > sizeof(d->name))
        return kIOReturnBadArgument;
    static const char kAirDropService[] = "wifid-assisted-discovery";
    static const char kSidecarService[] = "sidecar";
    auto nameMatches = [d](const char *name, size_t length) {
        if (d->name_len == length)
            return memcmp(d->name, name, length) == 0;
        return d->name_len == length + 1 && d->name[length] == '\0' &&
            memcmp(d->name, name, length) == 0;
    };
    uint8_t serviceBit = 0;
    if (nameMatches(kAirDropService, sizeof(kAirDropService) - 1))
        serviceBit = kAwdlTrafficAirDrop;
    else if (nameMatches(kSidecarService, sizeof(kSidecarService) - 1))
        serviceBit = kAwdlTrafficSidecar;
    else
        return kIOReturnSuccess;
    if (!fController || !_fCommandGate)
        return kIOReturnUnsupported;
    return _fCommandGate->runAction([](OSObject *, void *owner, void *arg,
        void *serviceArg, void *) -> IOReturn {
        auto that = static_cast<AirportItlwmAWDLInterface *>(owner);
        auto *registration = static_cast<apple80211_awdl_peer_traffic_registration *>(arg);
        auto *serviceBit = static_cast<uint8_t *>(serviceArg);
        IOReturn err = that->setAwdlTrafficReason(*serviceBit, registration->active != 0);
        if (err != kIOReturnSuccess)
            return err;
        if (*serviceBit == AirportItlwmAWDLInterface::kAwdlTrafficAirDrop)
            that->setProperty("AWDLDiscoveryActive", registration->active != 0);
        else
            that->setProperty("AWDLSidecarActive", registration->active != 0);
        return kIOReturnSuccess;
    }, this, d, &serviceBit);
}
AWDL_SET_STUB(AWDL_SECONDARY_MASTER_CHANNEL, apple80211_awdl_secondary_master_channel)
AWDL_SET_STUB(AWDL_MIN_RATE, apple80211_awdl_min_rate)
IOReturn AirportItlwmAWDLInterface::
setAWDL_ENCRYPTION_KEYS(apple80211_key *d)
{
    fCallCount++;
    if (!d || !fController || !awDataAllowed())
        return kIOReturnBadArgument;
    ieee80211com *ic = fController->fHalService->get80211Controller();
    if (!ic || !ic->ic_awdl_key || !_fCommandGate)
        return kIOReturnUnsupported;
    const bool remove = d->key_cipher_type == APPLE80211_CIPHER_NONE;
    if (!remove && (d->key_cipher_type != APPLE80211_CIPHER_AES_CCM || d->key_len != 16 ||
        d->key_index >= 4 || d->key_rsc_len > APPLE80211_RSC_LEN))
        return kIOReturnUnsupported;
    return _fCommandGate->runAction([](OSObject *, void *owner, void *arg, void *, void *) -> IOReturn {
        auto that = static_cast<AirportItlwmAWDLInterface *>(owner);
        auto *key = static_cast<apple80211_key *>(arg);
        ieee80211com *ic = that->fController->fHalService->get80211Controller();
        uint32_t flags = 0;
        if (key->key_flags & APPLE80211_KEY_FLAG_MULTICAST)
            flags |= IEEE80211_AWDL_KEY_GROUP;
        if (key->key_flags & APPLE80211_KEY_FLAG_TX)
            flags |= IEEE80211_AWDL_KEY_TX;
        if (key->key_flags & APPLE80211_KEY_FLAG_RX)
            flags |= IEEE80211_AWDL_KEY_RX;
        uint64_t rsc = key->key_rsc_len >= 6 ? LE_READ_6(key->key_rsc) : 0;
        int err = ic->ic_awdl_key(ic, key->key_ea.octet, (uint8_t)key->key_index,
            key->key, (uint8_t)key->key_len, flags, rsc,
            key->key_cipher_type == APPLE80211_CIPHER_NONE);
        return err ? kIOReturnError : kIOReturnSuccess;
    }, this, d);
}
IOReturn AirportItlwmAWDLInterface::
setAWDL_ENCRYPTION_TYPE(apple80211_encrypt *d)
{
    fCallCount++;
    if (!d || !fController || !_fCommandGate)
        return kIOReturnBadArgument;
    uint32_t type = reinterpret_cast<uint32_t *>(d)[1];
    if (type != APPLE80211_CIPHER_NONE && type != APPLE80211_CIPHER_AES_CCM)
        return kIOReturnUnsupported;
    return _fCommandGate->runAction([](OSObject *, void *owner, void *value, void *, void *) -> IOReturn {
        auto that = static_cast<AirportItlwmAWDLInterface *>(owner);
        ieee80211com *ic = that->fController->fHalService->get80211Controller();
        if (!ic || !ic->ic_awdl_set_encryption)
            return kIOReturnUnsupported;
        uint32_t type = (uint32_t)(uintptr_t)value;
        int err = ic->ic_awdl_set_encryption(ic, type);
        if (err)
            return kIOReturnError;
        that->fEncryptionType = type;
        return kIOReturnSuccess;
    }, this, (void *)(uintptr_t)type);
}
AWDL_SET_STUB(AWDL_TXCAL_PERIOD, apple80211_awdl_txcal_period)
AWDL_SET_STUB(AWDL_AWDL_ADVERTISERS, awdl_peer_advert_add)
AWDL_SET_STUB(AWDL_DFSP_CONFIG, apple80211_awdl_dfsp_config)
AWDL_SET_STUB(AWDL_DFSP_UCSA_CONFIG, apple80211_awdl_dfsp_ucsa_config)
IOReturn AirportItlwmAWDLInterface::
setCOMPANION_SKYWALK_IF_CREATE(apple80211_companion_skywalk_if_create *data)
{
    if (!data)
        return kIOReturnBadArgument;
    AirportItlwm *controller = OSDynamicCast(AirportItlwm, getController());
    if (!controller)
        return kIOReturnNotReady;
    // The companion interface (llw0, for Sidecar and Continuity Camera); only -itlawdlllw creates it.
    if (!itlAWDLLlwFlag()) {
        controller->setProperty("AWDLLlwStatus", "off (needs -itlawdlllw)");
        return kIOReturnSuccess;
    }
    uint8_t *request = reinterpret_cast<uint8_t *>(data);
    if (!_fCommandGate)
        return kIOReturnNotReady;
    return _fCommandGate->runAction([](OSObject *, void *owner, void *arg, void *, void *) -> IOReturn {
        auto *driver = static_cast<AirportItlwm *>(owner);
        auto *request = static_cast<uint8_t *>(arg);
        uint32_t operation;
        memcpy(&operation, request + 4, sizeof(operation));
        if (operation == 0) {
            if (driver->fLLWBsdIf && !driver->fLLWBsdIf->bsdRegistrationComplete())
                return kIOReturnBusy;
            return driver->destroyLowLatencyInterface();
        }
        if (operation != 1)
            return kIOReturnSuccess;
        return driver->createLowLatencyInterface(reinterpret_cast<char *>(request + 8));
    }, controller, request);
}
AWDL_SET_STUB(AWDL_OOB_SEC_PAYLOAD_REQUEST, apple80211_awdl_af_sec_payload_request)
IOReturn AirportItlwmAWDLInterface::
setLLW_PARAMS(apple80211_llw_params *d)
{
    fCallCount++;
    if (!d)
        return kIOReturnBadArgument;
    if (!fController || !_fCommandGate)
        return kIOReturnUnsupported;
    return _fCommandGate->runAction([](OSObject *, void *owner, void *arg, void *, void *) -> IOReturn {
        auto *that = static_cast<AirportItlwmAWDLInterface *>(owner);
        const auto *params = reinterpret_cast<const uint8_t *>(arg);
        // Apple CoreMac LLW_PARAMS stores its enable flag at byte offset 0x0b.
        return that->setAwdlTrafficReason(kAwdlTrafficLLWParams, params[0x0b] != 0);
    }, this, d);
}
AWDL_SET_STUB(TX_RATE, apple80211_tx_rate)
AWDL_SET_STUB(P2P_COEX_PROTECTION, apple80211_p2p_coex_protection)
AWDL_SET_STUB(PSF_DWELL, apple80211_awdl_psf_dwell_params)
AWDL_SET_STUB(AWDL_CONFIG, apple80211_awdl_config_request)
AWDL_SET_STUB(AWDL_RTG_PEER_STATS_CONFIG, apple80211_rtg_peer_stats_cfg)

#endif /* AIRPORT_WCL */
