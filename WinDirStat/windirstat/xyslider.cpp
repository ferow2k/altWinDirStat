// XySlider.cpp	- Implementation of CXySlider
//
// WinDirStat - Directory Statistics
// Copyright (C) 2003-2004 Bernhard Seifert
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: bseifert@users.sourceforge.net, bseifert@daccord.net
//
// Last modified: $Date$

#include "stdafx.h"
#include "xyslider.h"

#include "globalhelpers.h"

IMPLEMENT_DYNAMIC(CXySlider, CStatic)

//const UINT CXySlider::XY_SETPOS = WM_USER + 100;
//const UINT CXySlider::XY_GETPOS = WM_USER + 101;


void AFXAPI DDX_XySlider( CDataExchange* pDX, INT nIDC, WTL::CPoint& value ) {
	pDX->PrepareCtrl(nIDC);
	HWND hWndCtrl;
	pDX->m_pDlgWnd->GetDlgItem( nIDC, &hWndCtrl );
	if ( pDX->m_bSaveAndValidate ) {
		::SendMessageW( hWndCtrl, XY_GETPOS, 0, ( LPARAM ) &value );
		}
	else {
		::SendMessageW( hWndCtrl, XY_SETPOS, 0, ( LPARAM ) &value );
		}
	}

void CXySlider::Initialize( ) {
	if ( !m_inited && IsWindow( m_hWnd ) ) {
		// Make size odd, so that zero lines are central
		CRect rc;
		GetWindowRect( rc );
		GetParent( )->ScreenToClient( rc );
		if ( rc.Width( ) % 2 == 0 ) {
			rc.right--;
			}
		if ( rc.Height( ) % 2 == 0 ) {
			rc.bottom--;
			}
		MoveWindow( rc );

		// Initialize constants
		CalcSizes( );

		m_inited = true;
		}
	}

void CXySlider::CalcSizes( ) {
	//static const INT GRIPPER_RADIUS = 8;

	GetClientRect( m_rcAll );

	ASSERT( m_rcAll.left == 0 );
	ASSERT( m_rcAll.top  == 0 );
	ASSERT( m_rcAll.Width( )  % 2 == 1 );
	ASSERT( m_rcAll.Height( ) % 2 == 1 );
	ASSERT( m_rcAll.Width( )  >= GRIPPER_RADIUS * 2 );	// Control must be large enough
	ASSERT( m_rcAll.Height( ) >= GRIPPER_RADIUS * 2 );

	m_zero.x = m_rcAll.Width( )  / 2;
	m_zero.y = m_rcAll.Height( ) / 2;

	m_radius.cx = m_rcAll.Width( )  / 2 - 1;
	m_radius.cy = m_rcAll.Height( ) / 2 - 1;

	m_rcInner = m_rcAll;
	m_rcInner.DeflateRect( GRIPPER_RADIUS - 3, GRIPPER_RADIUS - 3 );

	m_gripperRadius.cx = GRIPPER_RADIUS;
	m_gripperRadius.cy = GRIPPER_RADIUS;

	m_range = m_radius - m_gripperRadius;
	}

void CXySlider::NotifyParent( ) const {
	NMHDR hdr;
	hdr.hwndFrom = m_hWnd;
	hdr.idFrom   = static_cast<UINT_PTR>( GetDlgCtrlID( ) );
	hdr.code     = XYSLIDER_CHANGED;
	TRACE( _T( "NotifyParent called! Sending WM_NOTIFY!\r\n" ) );
	GetParent( )->SendMessageW( WM_NOTIFY, static_cast<WPARAM>( GetDlgCtrlID( ) ), ( LPARAM ) &hdr );
	}

void CXySlider::PaintBackground( _In_ CDC& pdc ) {
	//ASSERT_VALID( pdc );
	pdc.FillSolidRect( m_rcAll, GetSysColor( COLOR_BTNFACE ) );

	CRect rc = m_rcInner;
	VERIFY( pdc.DrawEdge( rc, EDGE_SUNKEN, BF_RECT | BF_ADJUST ) );

	pdc.FillSolidRect( rc, RGB( 255, 255, 255 ) );

	CPen pen( PS_SOLID, 1, GetSysColor( COLOR_3DLIGHT ) );
	CSelectObject sopen( pdc, pen );

	pdc.MoveTo( rc.left,  m_zero.y );
	VERIFY( pdc.LineTo( rc.right, m_zero.y ) );
	pdc.MoveTo( m_zero.x, rc.top );
	VERIFY( pdc.LineTo( m_zero.x, rc.bottom ) );

	CRect circle = m_rcAll;
	circle.DeflateRect( m_gripperRadius );

	CSelectStockObject sobrush( pdc, NULL_BRUSH );
	VERIFY( pdc.Ellipse( circle ) );

	if ( GetFocus( ) == this ) {
		pdc.DrawFocusRect( m_rcAll );
		}
	}

void CXySlider::PaintGripper( _In_ CDC& pdc ) {
	//ASSERT_VALID( pdc );
	auto rc = GetGripperRect( );

	COLORREF color = GetSysColor( COLOR_BTNFACE );
	if ( m_gripperHighlight ) {
		INT r = GetRValue( color );
		INT g = GetGValue( color );
		INT b = GetBValue( color );;
		r += ( 255 - r ) / 3;
		g += ( 255 - g ) / 3;
		b += ( 255 - b ) / 3;
		color = RGB( r, g, b );
		}
	pdc.FillSolidRect( rc, color );
	VERIFY( pdc.DrawEdge( rc, EDGE_RAISED, BF_RECT ) );

	CPen pen( PS_SOLID, 1, GetSysColor( COLOR_3DSHADOW ) );
	CSelectObject sopen( pdc, pen );

	pdc.MoveTo( rc.left, rc.top + rc.Height( ) / 2 );
	VERIFY( pdc.LineTo( rc.right, rc.top + rc.Height( ) / 2 ) );
	pdc.MoveTo( rc.left + rc.Width( ) / 2, rc.top );
	VERIFY( pdc.LineTo( rc.left + rc.Width( ) / 2, rc.bottom ) );
	}

void CXySlider::DoMoveBy( _In_ const INT cx, _In_ const INT cy ) {
	m_pos.x += cx;
	CheckMinMax( m_pos.x, -m_range.cx, m_range.cx );

	m_pos.y += cy;
	CheckMinMax( m_pos.y, -m_range.cy, m_range.cy );

	VERIFY( RedrawWindow( ) );

	const WTL::CPoint oldpos = m_externalPos;
	InternToExtern( );
	if ( m_externalPos != oldpos ) {
		NotifyParent( );
		}
	}

void CXySlider::Handle_WM_MOUSEMOVE( _In_ const WTL::CPoint& ptMin, _In_ const WTL::CPoint& ptMax, _In_ const MSG& msg, _Inout_ WTL::CPoint& pt0 ) {
	WTL::CPoint pt = msg.pt;
	ScreenToClient( &pt );

	CheckMinMax( pt.x, ptMin.x, ptMax.x );
	CheckMinMax( pt.y, ptMin.y, ptMax.y );

	const INT dx = pt.x - pt0.x;
	const INT dy = pt.y - pt0.y;

	DoMoveBy( dx, dy );

	pt0 = pt;
	}

void CXySlider::DoDrag( _In_ const WTL::CPoint point ) {
	WTL::CPoint pt0 = point;

	HighlightGripper( true );

	/*
	inline CPoint CRect::CenterPoint() const throw()
	{
		return CPoint((left+right)/2, (top+bottom)/2);
	}	
	*/
	const auto grip_rect = GetGripperRect( );
	const auto grip_rect_center_x = ( ( grip_rect.left + grip_rect.right ) / 2 );
	const auto grip_rect_center_y = ( ( grip_rect.top + grip_rect.bottom ) / 2 );
	const auto new_point_x = ( pt0.x - grip_rect_center_x );
	const auto new_point_y = ( pt0.y - grip_rect_center_y );
	const CSize inGripper( new_point_x, new_point_y );
	const WTL::CPoint ptMin( m_zero - m_range + inGripper );
	const WTL::CPoint ptMax( m_zero + m_range + inGripper );

	SetCapture( );
	do {
		MSG msg;
		if ( !GetMessageW( &msg, NULL, 0, 0 ) ) {
			break;
			}

		if ( msg.message == WM_LBUTTONUP ) {
			break;
			}
		if ( GetCapture( ) != this ) {
			break;
			}

		if ( msg.message == WM_MOUSEMOVE ) {
			//CPoint pt = msg.pt;
			//ScreenToClient( &pt );
			//
			//CheckMinMax( pt.x, ptMin.x, ptMax.x );
			//CheckMinMax( pt.y, ptMin.y, ptMax.y );
			//
			//const INT dx = pt.x - pt0.x;
			//const INT dy = pt.y - pt0.y;
			//
			//DoMoveBy( dx, dy );
			//
			//pt0 = pt;
			Handle_WM_MOUSEMOVE( ptMin, ptMax, msg, pt0 );
			}
		else {
			DispatchMessageW( &msg );
			}
#pragma warning(suppress:4127)//conditional expression is constant
		} while ( true );

	VERIFY( ReleaseCapture( ) );

	HighlightGripper( false );
	}

void CXySlider::DoPage( _In_ const WTL::CPoint point ) {
	const WTL::CSize sz = point - ( m_zero + m_pos );

	ASSERT( sz.cx != 0 || sz.cy != 0 );

	const auto len = sqrt( static_cast<DOUBLE>( sz.cx ) * static_cast<DOUBLE>( sz.cx ) + static_cast<DOUBLE>( sz.cy ) * static_cast<DOUBLE>( sz.cy ) );

	const auto dx = static_cast<INT>( 10 * sz.cx / len );
	const auto dy = static_cast<INT>( 10 * sz.cy / len );

	DoMoveBy( dx, dy );
	}

void CXySlider::HighlightGripper( _In_ const bool on ) {
	m_gripperHighlight = on;
	VERIFY( RedrawWindow( ) );
	}

void CXySlider::RemoveTimer( ) {
	if ( m_timer != 0 ) {
		VERIFY( KillTimer( m_timer ) );
		}
	m_timer = 0;
	}

afx_msg void CXySlider::OnSetFocus( CWnd* pOldWnd ) {
	CStatic::OnSetFocus( pOldWnd );
	Invalidate( );
	}

afx_msg void CXySlider::OnKillFocus( CWnd* pNewWnd ) {
	CStatic::OnKillFocus( pNewWnd );
	Invalidate( );
	}



BEGIN_MESSAGE_MAP(CXySlider, CStatic)
	ON_WM_DESTROY()
	ON_WM_GETDLGCODE()
	ON_WM_NCHITTEST()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_PAINT()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_TIMER()
	ON_MESSAGE(XY_SETPOS, OnSetPos)
	ON_MESSAGE(XY_GETPOS, OnGetPos)
END_MESSAGE_MAP()

void CXySlider::OnPaint( ) {
	Initialize( );
	INT w = m_rcAll.Width( );
	INT h = m_rcAll.Height( );

	CPaintDC dc( this );
	CDC dcmem;
	VERIFY( dcmem.CreateCompatibleDC( &dc ) );
	CBitmap bm;
	VERIFY( bm.CreateCompatibleBitmap( &dc, w, h ) );
	CSelectObject sobm( dcmem, bm );

	PaintBackground( dcmem );
	// PaintValues(&dcmem); This is too noisy
	PaintGripper( dcmem );

	VERIFY( dc.BitBlt( 0, 0, w, h, &dcmem, 0, 0, SRCCOPY ) );
	//VERIFY( dcmem.DeleteDC( ) );
	}

void CXySlider::OnKeyDown( UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/ ) {
	switch ( nChar )
	{
		case VK_LEFT:
			return DoMoveBy( -1,  0 );
		case VK_RIGHT:
			return DoMoveBy(  1,  0 );
		case VK_UP:
			return DoMoveBy(  0, -1 );
		case VK_DOWN:
			return DoMoveBy(  0,  1 );
	}
	}

void CXySlider::OnLButtonDown( UINT /*nFlags*/, CPoint point ) {
	SetFocus( );

	auto rc = GetGripperRect( );

	if ( rc.PtInRect( point ) ) {
		return DoDrag( point );
		}
	DoPage( point );
	InstallTimer( );
	}

void CXySlider::OnLButtonDblClk( UINT /*nFlags*/, CPoint point ) {
	SetFocus( );

	if ( GetGripperRect( ).PtInRect( point ) ) {
		return DoMoveBy( -m_pos.x, -m_pos.y );
		}
	DoPage( point );
	InstallTimer( );
	}

void CXySlider::OnTimer( UINT_PTR /*nIDEvent*/ ) {
	WTL::CPoint point;
	VERIFY( GetCursorPos( &point ) );
	ScreenToClient( &point );

	auto rc = GetGripperRect( );
	if ( !rc.PtInRect( point ) ) {
		DoPage( point );
		}
	}

void CXySlider::SetPos( const WTL::CPoint pt ) {
	Initialize( );
	m_externalPos = pt;
	ExternToIntern( );
	Invalidate( );
	}

// $Log$
// Revision 1.4  2004/11/07 00:06:34  assarbad
// - Fixed minor bug with ampersand (details in changelog.txt)
//
// Revision 1.3  2004/11/05 16:53:08  assarbad
// Added Date and History tag where appropriate.
//
