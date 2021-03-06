/// @copyright
/// Copyright (C) 2020 Assured Information Security, Inc.
///
/// @copyright
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// @copyright
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// @copyright
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
/// SOFTWARE.

#ifndef VMEXIT_T_HPP
#define VMEXIT_T_HPP

#include <bf_debug_ops.hpp>
#include <bf_syscall_t.hpp>
#include <cpuid_commands.hpp>
#include <gs_t.hpp>
#include <intrinsic_t.hpp>
#include <tls_t.hpp>
#include <vp_pool_t.hpp>
#include <vps_pool_t.hpp>

#include <bsl/debug.hpp>
#include <bsl/discard.hpp>
#include <bsl/errc_type.hpp>
#include <bsl/safe_integral.hpp>
#include <bsl/unlikely_assert.hpp>

namespace integration
{
    /// @class integration::vmexit_t
    ///
    /// <!-- description -->
    ///   @brief Defines the extension's VMExit handler
    ///
    class vmexit_t final
    {
    public:
        /// <!-- description -->
        ///   @brief Initializes this vmexit_t.
        ///
        /// <!-- inputs/outputs -->
        ///   @param gs the gs_t to use
        ///   @param tls the tls_t to use
        ///   @param sys the bf_syscall_t to use
        ///   @param intrinsic the intrinsic_t to use
        ///   @param vp_pool the vp_pool_t to use
        ///   @param vps_pool the vps_pool_t to use
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     and friends otherwise
        ///
        [[nodiscard]] static constexpr auto
        initialize(
            gs_t &gs,
            tls_t &tls,
            syscall::bf_syscall_t &sys,
            intrinsic_t &intrinsic,
            vp_pool_t &vp_pool,
            vps_pool_t &vps_pool) noexcept -> bsl::errc_type
        {
            bsl::discard(tls);
            bsl::discard(intrinsic);
            bsl::discard(vp_pool);
            bsl::discard(vps_pool);

            gs.msr_bitmap = sys.bf_mem_op_alloc_page(gs.msr_bitmap_phys);
            if (bsl::unlikely_assert(nullptr == gs.msr_bitmap)) {
                bsl::print<bsl::V>() << bsl::here();
                return bsl::errc_failure;
            }

            return bsl::errc_success;
        }

        /// <!-- description -->
        ///   @brief Release the vmexit_t.
        ///
        /// <!-- inputs/outputs -->
        ///   @param gs the gs_t to use
        ///   @param tls the tls_t to use
        ///   @param sys the bf_syscall_t to use
        ///   @param intrinsic the intrinsic_t to use
        ///   @param vp_pool the vp_pool_t to use
        ///   @param vps_pool the vps_pool_t to use
        ///
        static constexpr void
        release(
            gs_t &gs,
            tls_t &tls,
            syscall::bf_syscall_t &sys,
            intrinsic_t &intrinsic,
            vp_pool_t &vp_pool,
            vps_pool_t &vps_pool) noexcept
        {
            bsl::errc_type ret{};

            bsl::discard(tls);
            bsl::discard(intrinsic);
            bsl::discard(vp_pool);
            bsl::discard(vps_pool);

            ret = sys.bf_mem_op_free_page(gs.msr_bitmap);
            if (bsl::unlikely_assert(!ret)) {
                bsl::print<bsl::V>() << bsl::here();
            }
            else {
                bsl::touch();
            }

            gs.msr_bitmap = {};
            gs.msr_bitmap_phys = {};
        }

        /// <!-- description -->
        ///   @brief Handle NMIs. This is required by Intel.
        ///
        /// <!-- inputs/outputs -->
        ///   @param gs the gs_t to use
        ///   @param tls the tls_t to use
        ///   @param sys the bf_syscall_t to use
        ///   @param intrinsic the intrinsic_t to use
        ///   @param vp_pool the vp_pool_t to use
        ///   @param vps_pool the vps_pool_t to use
        ///   @param vpsid the ID of the VPS that generated the VMExit
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     and friends otherwise
        ///
        [[nodiscard]] static constexpr auto
        handle_nmi(
            gs_t &gs,
            tls_t &tls,
            syscall::bf_syscall_t &sys,
            intrinsic_t &intrinsic,
            vp_pool_t &vp_pool,
            vps_pool_t &vps_pool,
            bsl::safe_uint16 const &vpsid) noexcept -> bsl::errc_type
        {
            bsl::discard(gs);
            bsl::discard(tls);
            bsl::discard(intrinsic);
            bsl::discard(vp_pool);
            bsl::discard(vps_pool);

            constexpr auto vmcs_procbased_ctls_idx{0x4002_u64};
            constexpr auto vmcs_set_nmi_window_exiting{0x400000_u32};

            bsl::safe_uint32 val{};
            bsl::errc_type ret{};

            val = sys.bf_vps_op_read32(vpsid, vmcs_procbased_ctls_idx);
            if (bsl::unlikely_assert(!val)) {
                bsl::print<bsl::V>() << bsl::here();
                return bsl::errc_failure;
            }

            val |= vmcs_set_nmi_window_exiting;

            ret = sys.bf_vps_op_write32(vpsid, vmcs_procbased_ctls_idx, val);
            if (bsl::unlikely_assert(!ret)) {
                bsl::print<bsl::V>() << bsl::here();
                return ret;
            }

            return sys.bf_vps_op_run_current();
        }

        /// <!-- description -->
        ///   @brief Handle NMIs Windows
        ///
        /// <!-- inputs/outputs -->
        ///   @param gs the gs_t to use
        ///   @param tls the tls_t to use
        ///   @param sys the bf_syscall_t to use
        ///   @param intrinsic the intrinsic_t to use
        ///   @param vp_pool the vp_pool_t to use
        ///   @param vps_pool the vps_pool_t to use
        ///   @param vpsid the ID of the VPS that generated the VMExit
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     and friends otherwise
        ///
        [[nodiscard]] static constexpr auto
        handle_nmi_window(
            gs_t &gs,
            tls_t &tls,
            syscall::bf_syscall_t &sys,
            intrinsic_t &intrinsic,
            vp_pool_t &vp_pool,
            vps_pool_t &vps_pool,
            bsl::safe_uint16 const &vpsid) noexcept -> bsl::errc_type
        {
            bsl::discard(gs);
            bsl::discard(tls);
            bsl::discard(intrinsic);
            bsl::discard(vp_pool);
            bsl::discard(vps_pool);

            constexpr auto vmcs_procbased_ctls_idx{0x4002_u64};
            constexpr auto vmcs_clear_nmi_window_exiting{0xFFBFFFFF_u32};

            bsl::safe_uint32 val{};
            bsl::errc_type ret{};

            val = sys.bf_vps_op_read32(vpsid, vmcs_procbased_ctls_idx);
            if (bsl::unlikely_assert(!val)) {
                bsl::print<bsl::V>() << bsl::here();
                return bsl::errc_failure;
            }

            val &= vmcs_clear_nmi_window_exiting;

            ret = sys.bf_vps_op_write32(vpsid, vmcs_procbased_ctls_idx, val);
            if (bsl::unlikely_assert(!ret)) {
                bsl::print<bsl::V>() << bsl::here();
                return ret;
            }

            constexpr auto vmcs_entry_interrupt_info_idx{0x4016_u64};
            constexpr auto vmcs_entry_interrupt_info_val{0x80000202_u32};

            ret = sys.bf_vps_op_write32(
                vpsid, vmcs_entry_interrupt_info_idx, vmcs_entry_interrupt_info_val);
            if (bsl::unlikely_assert(!ret)) {
                bsl::print<bsl::V>() << bsl::here();
                return ret;
            }

            return sys.bf_vps_op_run_current();
        }

        /// <!-- description -->
        ///   @brief Handles the CPUID VMexit
        ///
        /// <!-- inputs/outputs -->
        ///   @param gs the gs_t to use
        ///   @param tls the tls_t to use
        ///   @param sys the bf_syscall_t to use
        ///   @param intrinsic the intrinsic_t to use
        ///   @param vp_pool the vp_pool_t to use
        ///   @param vps_pool the vps_pool_t to use
        ///   @param vpsid the ID of the VPS that generated the VMExit
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     and friends otherwise
        ///
        [[nodiscard]] static constexpr auto
        handle_cpuid(
            gs_t &gs,
            tls_t &tls,
            syscall::bf_syscall_t &sys,
            intrinsic_t &intrinsic,
            vp_pool_t &vp_pool,
            vps_pool_t &vps_pool,
            bsl::safe_uint16 const &vpsid) noexcept -> bsl::errc_type
        {
            bsl::discard(gs);
            bsl::discard(tls);
            bsl::discard(vp_pool);
            bsl::discard(vps_pool);

            bsl::errc_type ret{};

            auto rax{sys.bf_tls_rax()};
            auto rbx{sys.bf_tls_rbx()};
            auto rcx{sys.bf_tls_rcx()};
            auto rdx{sys.bf_tls_rdx()};

            if (loader::CPUID_COMMAND_EAX == bsl::to_u32_unsafe(rax)) {
                switch (bsl::to_u32_unsafe(rcx).get()) {
                    case loader::CPUID_COMMAND_ECX_STOP.get(): {
                        sys.bf_tls_set_rax(loader::CPUID_COMMAND_RAX_SUCCESS);

                        ret = sys.bf_vps_op_advance_ip(vpsid);
                        if (bsl::unlikely_assert(!ret)) {
                            bsl::print<bsl::V>() << bsl::here();
                            return ret;
                        }

                        return sys.bf_vps_op_promote(vpsid);
                    }

                    case loader::CPUID_COMMAND_ECX_REPORT_ON.get(): {
                        break;
                    }

                    case loader::CPUID_COMMAND_ECX_REPORT_OFF.get(): {
                        break;
                    }

                    default: {
                        bsl::error() << "unsupported cpuid command "    // --
                                     << bsl::hex(rcx)                   // --
                                     << bsl::endl                       // --
                                     << bsl::here();                    // --

                        break;
                    }
                }

                return sys.bf_vps_op_advance_ip_and_run_current();
            }

            intrinsic.cpuid(rax, rbx, rcx, rdx);

            sys.bf_tls_set_rax(rax);
            sys.bf_tls_set_rbx(rbx);
            sys.bf_tls_set_rcx(rcx);
            sys.bf_tls_set_rdx(rdx);

            return sys.bf_vps_op_advance_ip_and_run_current();
        }

        /// <!-- description -->
        ///   @brief Dispatches the VMExit.
        ///
        /// <!-- inputs/outputs -->
        ///   @param gs the gs_t to use
        ///   @param tls the tls_t to use
        ///   @param sys the bf_syscall_t to use
        ///   @param intrinsic the intrinsic_t to use
        ///   @param vp_pool the vp_pool_t to use
        ///   @param vps_pool the vps_pool_t to use
        ///   @param vpsid the ID of the VPS that generated the VMExit
        ///   @param exit_reason the exit reason associated with the VMExit
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     and friends otherwise
        ///
        [[nodiscard]] static constexpr auto
        dispatch(
            gs_t &gs,
            tls_t &tls,
            syscall::bf_syscall_t &sys,
            intrinsic_t &intrinsic,
            vp_pool_t &vp_pool,
            vps_pool_t &vps_pool,
            bsl::safe_uint16 const &vpsid,
            bsl::safe_uint64 const &exit_reason) noexcept -> bsl::errc_type
        {
            constexpr auto exit_reason_nmi{0x0_u64};
            constexpr auto exit_reason_nmi_window{0x8_u64};
            constexpr auto exit_reason_cpuid{0xA_u64};

            switch (exit_reason.get()) {
                case exit_reason_nmi.get(): {
                    return handle_nmi(gs, tls, sys, intrinsic, vp_pool, vps_pool, vpsid);
                }

                case exit_reason_nmi_window.get(): {
                    return handle_nmi_window(gs, tls, sys, intrinsic, vp_pool, vps_pool, vpsid);
                }

                case exit_reason_cpuid.get(): {
                    return handle_cpuid(gs, tls, sys, intrinsic, vp_pool, vps_pool, vpsid);
                }

                default: {
                    break;
                }
            }

            bsl::error() << "unsupported vmexit "    // --
                         << bsl::hex(exit_reason)    // --
                         << bsl::endl                // --
                         << bsl::here();             // --

            return bsl::errc_failure;
        }
    };
}

#endif
