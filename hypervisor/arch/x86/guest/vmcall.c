/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <types.h>
#include <errno.h>
#include <spinlock.h>
#include <vcpu.h>
#include <vm.h>
#include <acrn_hv_defs.h>
#include <hypercall.h>
#include <trace.h>
#include <logmsg.h>

static spinlock_t vmm_hypercall_lock = {
	.head = 0U,
	.tail = 0U,
};

static int32_t dispatch_hypercall(struct acrn_vcpu *vcpu)
{
	struct acrn_vm *vm = vcpu->vm;
	/* hypercall ID from guest*/
	uint64_t hypcall_id = vcpu_get_gpreg(vcpu, CPU_REG_R8);
	/* hypercall param1 from guest*/
	uint64_t param1 = vcpu_get_gpreg(vcpu, CPU_REG_RDI);
	/* hypercall param2 from guest*/
	uint64_t param2 = vcpu_get_gpreg(vcpu, CPU_REG_RSI);
	/* in case hypercall param1 is a vm id */
	uint16_t vm_id = (uint16_t)param1;
	bool vmid_is_valid = (vm_id < CONFIG_MAX_VM_NUM) ? true : false;
	int32_t ret = -1;

	switch (hypcall_id) {
	case HC_SOS_OFFLINE_CPU:
		spinlock_obtain(&vmm_hypercall_lock);
		ret = hcall_sos_offline_cpu(vm, param1);
		spinlock_release(&vmm_hypercall_lock);
		break;
	case HC_GET_API_VERSION:
		ret = hcall_get_api_version(vm, param1);
		break;

	case HC_GET_PLATFORM_INFO:
		ret = hcall_get_platform_info(vm, param1);
		break;

	case HC_SET_CALLBACK_VECTOR:
		ret = hcall_set_callback_vector(vm, param1);

		break;

	case HC_CREATE_VM:
		spinlock_obtain(&vmm_hypercall_lock);
		ret = hcall_create_vm(vm, param1);
		spinlock_release(&vmm_hypercall_lock);
		break;

	case HC_DESTROY_VM:
		/* param1: vmid */
		if (vmid_is_valid) {
			spinlock_obtain(&vmm_hypercall_lock);
			ret = hcall_destroy_vm(vm_id);
			spinlock_release(&vmm_hypercall_lock);
		}
		break;

	case HC_START_VM:
		/* param1: vmid */
		if (vmid_is_valid) {
			spinlock_obtain(&vmm_hypercall_lock);
			ret = hcall_start_vm(vm_id);
			spinlock_release(&vmm_hypercall_lock);
		}
		break;

	case HC_RESET_VM:
		/* param1: vmid */
		if (vmid_is_valid) {
			spinlock_obtain(&vmm_hypercall_lock);
			ret = hcall_reset_vm(vm_id);
			spinlock_release(&vmm_hypercall_lock);
		}
		break;

	case HC_PAUSE_VM:
		/* param1: vmid */
		if (vmid_is_valid) {
			spinlock_obtain(&vmm_hypercall_lock);
			ret = hcall_pause_vm(vm_id);
			spinlock_release(&vmm_hypercall_lock);
		}
		break;

	case HC_CREATE_VCPU:
		/* param1: vmid */
		if (vmid_is_valid) {
			spinlock_obtain(&vmm_hypercall_lock);
			ret = hcall_create_vcpu(vm, vm_id, param2);
			spinlock_release(&vmm_hypercall_lock);
		}
		break;

	case HC_SET_VCPU_REGS:
		/* param1: vmid */
		if (vmid_is_valid) {
			spinlock_obtain(&vmm_hypercall_lock);
			ret = hcall_set_vcpu_regs(vm, vm_id, param2);
			spinlock_release(&vmm_hypercall_lock);
		}
		break;

	case HC_SET_IRQLINE:
		/* param1: vmid */
		if (vmid_is_valid) {
			ret = hcall_set_irqline(vm, vm_id,
					(struct acrn_irqline_ops *)&param2);
		}
		break;

	case HC_INJECT_MSI:
		/* param1: vmid */
		if (vmid_is_valid) {
			ret = hcall_inject_msi(vm, vm_id, param2);
		}
		break;

	case HC_SET_IOREQ_BUFFER:
		/* param1: vmid */
		if (vmid_is_valid) {
			spinlock_obtain(&vmm_hypercall_lock);
			ret = hcall_set_ioreq_buffer(vm, vm_id, param2);
			spinlock_release(&vmm_hypercall_lock);
		}
		break;

	case HC_NOTIFY_REQUEST_FINISH:
		/* param1: vmid
		 * param2: vcpu_id */
		if (vmid_is_valid) {
			ret = hcall_notify_ioreq_finish(vm_id,
				(uint16_t)param2);
		}
		break;

	case HC_VM_SET_MEMORY_REGIONS:
		ret = hcall_set_vm_memory_regions(vm, param1);
		break;

	case HC_VM_WRITE_PROTECT_PAGE:
		/* param1: vmid */
		if (vmid_is_valid) {
			ret = hcall_write_protect_page(vm, vm_id, param2);
		}
		break;

	/*
	 * Don't do MSI remapping and make the pmsi_data equal to vmsi_data
	 * This is a temporary solution before this hypercall is removed from SOS
	 */
	case HC_VM_PCI_MSIX_REMAP:
		ret = 0;
		break;

	case HC_VM_GPA2HPA:
		/* param1: vmid */
		if (vmid_is_valid) {
			ret = hcall_gpa_to_hpa(vm, vm_id, param2);
		}
		break;

	case HC_ASSIGN_PTDEV:
		/* param1: vmid */
		if (vmid_is_valid) {
			ret = hcall_assign_ptdev(vm, vm_id, param2);
		}
		break;

	case HC_DEASSIGN_PTDEV:
		/* param1: vmid */
		if (vmid_is_valid) {
			ret = hcall_deassign_ptdev(vm, vm_id, param2);
		}
		break;

	case HC_SET_PTDEV_INTR_INFO:
		/* param1: vmid */
		if (vmid_is_valid) {
			ret = hcall_set_ptdev_intr_info(vm, vm_id, param2);
		}
		break;

	case HC_RESET_PTDEV_INTR_INFO:
		/* param1: vmid */
		if (vmid_is_valid) {
			ret = hcall_reset_ptdev_intr_info(vm, vm_id, param2);
		}
		break;

	case HC_WORLD_SWITCH:
		ret = hcall_world_switch(vcpu);
		break;

	case HC_INITIALIZE_TRUSTY:
		ret = hcall_initialize_trusty(vcpu, param1);
		break;

	case HC_PM_GET_CPU_STATE:
		ret = hcall_get_cpu_pm_state(vm, param1, param2);
		break;

	case HC_SAVE_RESTORE_SWORLD_CTX:
		ret = hcall_save_restore_sworld_ctx(vcpu);
		break;

	case HC_VM_INTR_MONITOR:
		/* param1: vmid */
		if (vmid_is_valid) {
			ret = hcall_vm_intr_monitor(vm, vm_id, param2);
		}
		break;

	default:
		ret = hcall_debug(vm, param1, param2, hypcall_id);
		break;
	}

	return ret;
}

/*
 * Pass return value to SOS by register rax.
 * This function should always return 0 since we shouldn't
 * deal with hypercall error in hypervisor.
 */
int32_t vmcall_vmexit_handler(struct acrn_vcpu *vcpu)
{
	int32_t ret;
	struct acrn_vm *vm = vcpu->vm;
	/* hypercall ID from guest*/
	uint64_t hypcall_id = vcpu_get_gpreg(vcpu, CPU_REG_R8);

	if (!is_sos_vm(vm) && (hypcall_id != HC_WORLD_SWITCH) &&
		(hypcall_id != HC_INITIALIZE_TRUSTY) &&
		(hypcall_id != HC_SAVE_RESTORE_SWORLD_CTX)) {
		vcpu_inject_ud(vcpu);
		pr_err("hypercall %d is only allowed from SOS_VM!\n", hypcall_id);
	} else if (!is_hypercall_from_ring0()) {
		pr_err("hypercall is only allowed from RING-0!\n");
	        ret = -EACCES;
		vcpu_set_gpreg(vcpu, CPU_REG_RAX, (uint64_t)ret);
	} else {
		/* Dispatch the hypercall handler */
		ret = dispatch_hypercall(vcpu);
		vcpu_set_gpreg(vcpu, CPU_REG_RAX, (uint64_t)ret);
	}

	TRACE_2L(TRACE_VMEXIT_VMCALL, vm->vm_id, hypcall_id);

	return 0;
}
