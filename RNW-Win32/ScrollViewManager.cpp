#include "pch.h"
#include "ScrollViewManager.h"
#include "PaperUIManager.h"

RECT RectFromYogaNode(YGNodeRef node)
{
	return RECT{
		static_cast<LONG>(YGNodeLayoutGetLeft(node)),
		static_cast<LONG>(YGNodeLayoutGetTop(node)),
		static_cast<LONG>(YGNodeLayoutGetLeft(node) + YGNodeLayoutGetWidth(node)),
		static_cast<LONG>(YGNodeLayoutGetTop(node) + YGNodeLayoutGetHeight(node)),
	};
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
		auto p = LOWORD(wParam);
		auto dy = 0;
		switch (p)
		{
		case SB_LINEDOWN: dy = 1; break;
		case SB_LINEUP: dy = -1; break;
		case SB_PAGEDOWN: dy = 10; break;
		case SB_PAGEUP: dy -= 10; break;
		case SB_TOP: dy = 0; break;
		case SB_BOTTOM: dy = INT_MAX; break;
		}

		SCROLLINFO si{ sizeof(SCROLLINFO) };

		si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE | SIF_TRACKPOS;
		GetScrollInfo(window, SB_VERT, &si);
		si.nPos += dy;
		//HWND childWnd{ nullptr };
		//if (m_children.size() != 0)
		//{
		//	if (auto child = m_children[0].lock())
		//	{
		//		childWnd = child->window;
		//	}
		//}
		SetScrollInfo(window, SB_VERT, &si, true);

		ScrollWindowEx(window, 0, -dy, nullptr, nullptr, nullptr, nullptr, SW_SCROLLCHILDREN);
		RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE);
		return 0;
	}

	case WM_SIZE:
	{
		SCROLLINFO si{ sizeof(SCROLLINFO) };
		auto w = YGNodeStyleGetWidth(yogaNode.get());
		auto h = YGNodeStyleGetHeight(yogaNode.get());
		RECT r{};
		for (auto& child : m_children)
		{
			if (auto c = child.lock())
			{
				auto childMeasure = c->Measure(0, YGMeasureModeExactly, 0, YGMeasureModeExactly);
				auto childRect = RectFromYogaNode(c->yogaNode.get());
				UnionRect(&r, &r, &childRect);
			}
		}
		auto measure = Measure(0, YGMeasureModeExactly, 0, YGMeasureModeExactly);
		si.fMask = SIF_RANGE;
		si.nMin = 0;
//		si.nPos = 0;
		si.nMax = measure.height ? measure.height : (r.bottom - r.top);
		SetScrollInfo(window, SB_VERT, &si, TRUE);
		EnableScrollBar(window, SB_VERT, ESB_ENABLE_BOTH);

		break;
	}
	}

	return ShadowNode::WndProc(msg, wParam, lParam);
}