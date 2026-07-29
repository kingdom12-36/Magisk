package com.shadowmask.ui.log

import android.os.Bundle
import android.view.Menu
import android.view.MenuInflater
import android.view.MenuItem
import android.view.View
import android.widget.HorizontalScrollView
import androidx.core.view.MenuProvider
import androidx.core.view.isVisible
import com.shadowmask.R
import com.shadowmask.arch.BaseFragment
import com.shadowmask.arch.viewModel
import com.shadowmask.databinding.FragmentLogMd2Binding
import com.shadowmask.ui.MainActivity
import com.shadowmask.utils.AccessibilityUtils
import com.shadowmask.utils.MotionRevealHelper
import rikka.recyclerview.addEdgeSpacing
import rikka.recyclerview.addItemSpacing
import rikka.recyclerview.fixEdgeEffect
import com.shadowmask.core.R as CoreR

class LogFragment : BaseFragment<FragmentLogMd2Binding>(), MenuProvider {

    override val layoutRes = R.layout.fragment_log_md2
    override val viewModel by viewModel<LogViewModel>()
    override val snackbarView: View?
        get() = if (isShadowMaskLogVisible) binding.logFilterSuperuser.snackbarContainer
                else super.snackbarView
    override val snackbarAnchorView get() = binding.logFilterToggle

    private var actionSave: MenuItem? = null
    private var isShadowMaskLogVisible
        get() = binding.logFilter.isVisible
        set(value) {
            MotionRevealHelper.withViews(binding.logFilter, binding.logFilterToggle, value)
            actionSave?.isVisible = !value
            with(activity as MainActivity) {
                invalidateToolbar()
                requestNavigationHidden(value)
                setDisplayHomeAsUpEnabled(value)
            }
        }

    override fun onStart() {
        super.onStart()
        activity?.setTitle(CoreR.string.logs)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        binding.logFilterToggle.setOnClickListener {
            isShadowMaskLogVisible = true
        }

        binding.logFilterSuperuser.logSuperuser.apply {
            addEdgeSpacing(bottom = R.dimen.l1)
            addItemSpacing(R.dimen.l1, R.dimen.l_50, R.dimen.l1)
            fixEdgeEffect()
        }

        if (!AccessibilityUtils.isAnimationEnabled(requireContext().contentResolver)) {
            val scrollView = view.findViewById<HorizontalScrollView>(R.id.log_scroll_shadowmask)
            scrollView.setOverScrollMode(View.OVER_SCROLL_NEVER)
        }
    }


    override fun onCreateMenu(menu: Menu, inflater: MenuInflater) {
        inflater.inflate(R.menu.menu_log_md2, menu)
        actionSave = menu.findItem(R.id.action_save)?.also {
            it.isVisible = !isShadowMaskLogVisible
        }
    }

    override fun onMenuItemSelected(item: MenuItem): Boolean {
        when (item.itemId) {
            R.id.action_save -> viewModel.saveShadowMaskLog()
            R.id.action_clear ->
                if (!isShadowMaskLogVisible) viewModel.clearShadowMaskLog()
                else viewModel.clearLog()
        }
        return super.onOptionsItemSelected(item)
    }


    override fun onPreBind(binding: FragmentLogMd2Binding) = Unit

    override fun onBackPressed(): Boolean {
        if (binding.logFilter.isVisible) {
            isShadowMaskLogVisible = false
            return true
        }
        return super.onBackPressed()
    }

}
