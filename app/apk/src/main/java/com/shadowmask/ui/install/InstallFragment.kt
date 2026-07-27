package com.shadowmask.ui.install

import com.shadowmask.R
import com.shadowmask.arch.BaseFragment
import com.shadowmask.arch.viewModel
import com.shadowmask.databinding.FragmentInstallMd2Binding
import com.shadowmask.core.R as CoreR

class InstallFragment : BaseFragment<FragmentInstallMd2Binding>() {

    override val layoutRes = R.layout.fragment_install_md2
    override val viewModel by viewModel<InstallViewModel>()

    override fun onStart() {
        super.onStart()
        requireActivity().setTitle(CoreR.string.install)
    }
}
