#include "pch.h"
#include "ScrollViewManager.h"
#include "PaperUIManager.h"

YGSize ScrollContentViewShadowNode::Measure(float width,
	YGMeasureMode widthMode,
	float height,
	YGMeasureMode heightMode)
{
	// assume vertical scroll for now
	YGSize r{};
	for (auto& child : m_children)
	{
		if (auto c = child.lock())
		{
			auto childMeasure = c->Measure(width, widthMode, height, heightMode);
			r.height += childMeasure.height;
			r.width = std::max(r.width, childMeasure.width);
		}
	}
	return r;
}

LRESULT ScrollViewShadowNode::WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	auto style = GetWindowLong(window, GWL_STYLE);
	switch (msg)
	{
	case WM_NCHITTEST:
	{
		RECT rect{};
		GetWindowRect(window, &rect);
		auto x = LOWORD(lParam);
		if (x > rect.right - 20)
		{
			return HTVSCROLL;
		}
		break;
	}
	case WM_VSCROLL:
	{
		const auto p = LOWORD(wParam);
		auto dy = 0;
		constexpr auto LineScrollPx = 10;
		constexpr auto PageScrollPx = 50;
		switch (p)
		{
		case SB_LINEDOWN: dy = LineScrollPx; break;
		case SB_LINEUP: dy = -LineScrollPx; break;
		case SB_PAGEDOWN: dy = PageScrollPx; break;
		case SB_PAGEUP: dy = -PageScrollPx; break;
		case SB_TOP: dy = 0; break;
		case SB_BOTTOM: dy = INT_MAX; break;
		case SB_THUMBTRACK: return 0;
		default: break;
		}

		SCROLLINFO si{ sizeof(SCROLLINFO) };

		si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE | SIF_TRACKPOS;
		GetScrollInfo(window, SB_VERT, &si);
		const auto oldPos = si.nPos;
		if (p == SB_THUMBPOSITION) {
			si.nPos = HIWORD(wParam);
		}
		else
		{
			si.nPos += dy;
		}

		dy = si.nPos - oldPos;
		const auto newPos = SetScrollInfo(window, SB_VERT, &si, true);
		cdbg << "Scroll by " << dy << "  tag  " << IWin32ViewManager::GetTag(window) << "\n";
		if (newPos != oldPos)
		{
			ScrollWindowEx(window, 0, -dy, nullptr, nullptr, nullptr, nullptr, SW_SCROLLCHILDREN);
			RedrawWindow(::GetParent(window), nullptr, nullptr, RDW_INVALIDATE); // we need to redraw the parent's background
		}
		return 0;
	}

	}

	return ShadowNode::WndProc(msg, wParam, lParam);
}

void ScrollViewShadowNode::UpdateLayout(float left, float top, float width, float height)
{
	if (!isnan(width) && !isnan(height))
	{
		ShadowNode::UpdateLayout(left, top, width, height);

		SCROLLINFO si{ sizeof(SCROLLINFO) };

		si.fMask = SIF_RANGE | SIF_POS;
		si.nMin = 0;
		si.nPos = 0;
		si.nMax = static_cast<int>(height);
		SetScrollInfo(window, SB_VERT, &si, TRUE);
		EnableScrollBar(window, SB_VERT, ESB_ENABLE_BOTH);

		float top = 0;
		for (const auto c : m_children)
		{
			if (const auto child = c.lock())
			{
				auto childMeasure = child->Measure(width, YGMeasureModeAtMost, height, YGMeasureModeAtMost);
				child->UpdateLayout(0, top, childMeasure.width, childMeasure.height);
				top += childMeasure.height;
			}
		}
	}
}