// graphview.cpp: Implementation of CGraphView
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
//#include "dirstatview.h"
//#include "item.h"

//#include ".\graphview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE(CGraphView, CView)

BEGIN_MESSAGE_MAP(CGraphView, CView)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_SETFOCUS()
	ON_WM_CONTEXTMENU()
	ON_WM_MOUSEMOVE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_COMMAND(ID_POPUP_CANCEL, OnPopupCancel)
END_MESSAGE_MAP()


//CGraphView::CGraphView( ) : m_recalculationSuspended( false ), m_showTreemap( true ), m_size.cx( 0 ), m_size.cy( 0 ), m_dimmedSize.cx( 0 ), m_dimmedSize.cy( 0 ), m_timer( 0 ) { }

void CGraphView::TreemapDrawingCallback( ) {
	GetApp( )->PeriodicalUpdateRamUsage( );
	}

void CGraphView::SuspendRecalculation( _In_ bool suspend ) {
	m_recalculationSuspended = suspend;
	if ( !suspend ) {
		Invalidate( );
		}
	}

bool CGraphView::IsShowTreemap( ) {
	return m_showTreemap;
	}

void CGraphView::ShowTreemap( _In_ bool show ) {
	m_showTreemap = show;
	}

BOOL CGraphView::PreCreateWindow( CREATESTRUCT& cs ) {
	// We don't want a background brush
	VERIFY( CView::PreCreateWindow( cs ) ); // this registers a wndclass
	
	WNDCLASS wc;
	VERIFY( GetClassInfo( AfxGetInstanceHandle( ), cs.lpszClass, &wc ) );
	wc.hbrBackground = NULL;
	wc.lpszClassName = _T( "windirstat_graphview_class" );
	cs.lpszClass = ( LPCTSTR ) RegisterClass( &wc );
	
	return true;
	}

void CGraphView::OnInitialUpdate( ) {
	CView::OnInitialUpdate( );
	}

void CGraphView::DrawEmptyView( ) {
	CClientDC dc(this);
	DrawEmptyView(&dc);
	}

void CGraphView::DrawEmptyView( _In_ CDC *pDC ) {
	ASSERT_VALID( pDC );
	const COLORREF gray = RGB( 160, 160, 160 );
	Inactivate( );

	CRect rc;
	GetClientRect( rc );

	if ( m_dimmed.m_hObject == NULL ) {
		return pDC->FillSolidRect( rc, gray );
		}
	CDC dcmem;
	dcmem.CreateCompatibleDC( pDC );
	CSelectObject sobmp( &dcmem, &m_dimmed );
	pDC->BitBlt( rc.left, rc.top, m_dimmedSize.cx, m_dimmedSize.cy, &dcmem, 0, 0, SRCCOPY );

	if ( rc.Width( ) > m_dimmedSize.cx ) {
		CRect r = rc;
		r.left = r.left + m_dimmedSize.cx;
		pDC->FillSolidRect( r, gray );
		}

	if ( rc.Height( ) > m_dimmedSize.cy ) {
		CRect r = rc;
		r.top = r.top + m_dimmedSize.cy;
		pDC->FillSolidRect( r, gray );
		}
	}

void CGraphView::DoDraw( _In_ CDC* pDC, _In_ CDC& dcmem, _In_ CRect& rc ) {
	//LockWindowUpdate( );
	CWaitCursor wc;

	m_bitmap.CreateCompatibleBitmap( pDC, m_size.cx, m_size.cy );

	CSelectObject sobmp( &dcmem, &m_bitmap );
	auto Document = static_cast< CDirstatDoc* >( m_pDocument );
	if ( Document != NULL ) {
		if ( Document->IsZoomed( ) ) {
			DrawZoomFrame( &dcmem, rc );
			}
		auto Options = GetOptions( );
		if ( Options != NULL ) {
			m_treemap.DrawTreemap( &dcmem, rc, Document->GetZoomItem( ), Options->GetTreemapOptions( ) );
#ifdef _DEBUG
			m_treemap.RecurseCheckTree( Document->GetRootItem( ) );
#endif
			}
		ASSERT( Options != NULL ); //fall back to default options?
		}
	ASSERT( Document != NULL );
	//UnlockWindowUpdate( );
	// Cause OnIdle() to be called once.
	PostAppMessage( GetCurrentThreadId( ), WM_NULL, 0, 0 );
	}

void CGraphView::DrawViewNotEmpty( _In_ CDC* pDC ) {
	CRect rc;
	GetClientRect( rc );
	ASSERT( m_size == rc.Size( ) );
	ASSERT( rc.TopLeft( ) == CPoint( 0, 0 ) );

	CDC dcmem;
	dcmem.CreateCompatibleDC( pDC );

	if ( !IsDrawn( ) ) {
		DoDraw( pDC, dcmem, rc );
		}

	CSelectObject sobmp2( &dcmem, &m_bitmap );
	pDC->BitBlt( 0, 0, m_size.cx, m_size.cy, &dcmem, 0, 0, SRCCOPY );

	DrawHighlights( pDC );
	
	}

void CGraphView::OnDraw( CDC* pDC ) {
	ASSERT_VALID( pDC );
	auto aDocument = static_cast< CDirstatDoc* >( m_pDocument );
	if ( aDocument != NULL ) {
		auto root = aDocument->GetRootItem( );
		if ( root != NULL && root->IsDone( ) ) {
			if ( m_recalculationSuspended || !m_showTreemap ) {
				// TODO: draw something interesting, e.g. outline of the first level.
				DrawEmptyView( pDC );
				}
			else {
				DrawViewNotEmpty( pDC );
				}
			}
		else {
			DrawEmptyView( pDC );
			}
		}
	ASSERT( aDocument != NULL );
	}

void CGraphView::DrawZoomFrame( _In_ CDC *pdc, _In_ CRect& rc ) {
	ASSERT_VALID( pdc );
	const INT w = 4;
	CRect r;
	
	r = rc;
	r.bottom = r.top + w;
	auto Document = static_cast< CDirstatDoc* >( m_pDocument );
	if ( Document != NULL ) {
		pdc->FillSolidRect( r, Document->GetZoomColor( ) );

		r = rc;
		r.top = r.bottom - w;
		pdc->FillSolidRect( r, Document->GetZoomColor( ) );

		r = rc;
		r.right = r.left + w;
		pdc->FillSolidRect( r, Document->GetZoomColor( ) );

		r = rc;
		r.left = r.right - w;
		pdc->FillSolidRect( r, Document->GetZoomColor( ) );

		rc.DeflateRect( w, w );
		}
	else {
		ASSERT( Document != NULL );
		//Fall back to some sane defaults?
		r = rc;
		r.top = r.bottom - w;

		r = rc;
		r.right = r.left + w;

		r = rc;
		r.left = r.right - w;

		rc.DeflateRect( w, w );

		}
	}

void CGraphView::DrawHighlights( _In_ CDC *pdc ) {
	ASSERT_VALID( pdc );
	//std::future<bool> fut = std::async( std::launch::async | std::launch::deferred, [] {return ( GetApp( )->b_PeriodicalUpdateRamUsage( ) ); } );
	
	switch ( GetMainFrame( )->GetLogicalFocus( ) )
	{
		case LF_DIRECTORYLIST:
			DrawSelection( pdc );
			break;
		case LF_EXTENSIONLIST:
			DrawHighlightExtension( pdc );
			break;
	}
	GetApp( )->b_PeriodicalUpdateRamUsage( );
	//fut.get( );
	}

void CGraphView::DrawHighlightExtension( _In_ CDC* pdc ) {
	ASSERT_VALID( pdc );
	CWaitCursor wc;

	CPen pen( PS_SOLID, 1, GetOptions( )->GetTreemapHighlightColor( ) );
	CSelectObject sopen( pdc, &pen );
	CSelectStockObject sobrush( pdc, NULL_BRUSH );
	auto Document = static_cast< CDirstatDoc* >( m_pDocument );;
	if ( Document == NULL ) {
		ASSERT( Document != NULL );
		return;
		}
	RecurseHighlightExtension( pdc, Document->GetZoomItem( ), Document->GetHighlightExtension( ) );
	//std::future<void> futr = std::async( std::launch::async, [ this, pdc, Document ] { RecurseHighlightExtension( pdc, Document->GetZoomItem( ), Document->GetHighlightExtension( ) ); } );
	//futr.get( );
	}

void CGraphView::RecurseHighlightExtension( _In_ CDC *pdc, _In_ const CItemBranch* item ) {
	ASSERT_VALID( pdc );
	auto rc = item->TmiGetRectangle( );
	if ( ( rc.Width( ) ) <= 0 || ( rc.Height( ) ) <= 0 ) {
		return;
		}
	
	if ( item->TmiIsLeaf( ) ) {
		if ( item->GetType( ) == IT_FILE && item->GetExtension( ).CompareNoCase( ( static_cast< CDirstatDoc* >( m_pDocument ) )->GetHighlightExtension( ) ) == 0 ) {
			return RenderHighlightRectangle( pdc, rc );
			}
		return;
		}
	for ( auto& aChild : item->m_children ) {
		if ( aChild->m_size == 0 ) {
			ASSERT( std::uint64_t( aChild->TmiGetSize( ) ) == aChild->m_size );
			break;
			}
		if ( aChild->m_rect.left == -1 ) {
			break;
			}
		RecurseHighlightExtension( pdc, aChild );
		}
	}

void CGraphView::RecurseHighlightChildren( _In_ CDC* pdc, _In_ const CItemBranch* item, _In_z_ PCWSTR ext ) {
	for ( auto& child : item->m_children ) {
		RecurseHighlightExtension( pdc, child, ext );
		}
	}

void CGraphView::RecurseHighlightExtension( _In_ CDC* pdc, _In_ const CItemBranch* item, _In_z_ PCWSTR ext ) {
	ASSERT_VALID( pdc );
	auto rc = item->m_rect;
	if ( ( rc.right - rc.left ) <= 0 || ( rc.bottom - rc.top ) <= 0 ) {
		return;
		}
	
	if ( item->TmiIsLeaf( ) ) {
		if ( item->m_type == IT_FILE ) {
			auto extensionStrPtr = item->CStyle_GetExtensionStrPtr( );
			auto scmp = wcscmp( extensionStrPtr, ext );
			if ( scmp == 0 ) {
				auto rect = item->TmiGetRectangle( );
				return RenderHighlightRectangle( pdc, rect );
				}
			return;
			}
		}
	RecurseHighlightChildren( pdc, item, ext );
	}

void CGraphView::TweakSizeOfRectangleForHightlight( _In_ CRect& rc, _In_ CRect& rcClient ) {
	if ( m_treemap.GetOptions( ).grid ) {
		rc.right++;
		rc.bottom++;
		}
	if ( rcClient.left < rc.left ) {
		rc.left--;
		}
	if ( rcClient.top < rc.top ) {
		rc.top--;
		}
	if ( rc.right < rcClient.right ) {
		rc.right++;
		}
	if ( rc.bottom < rcClient.bottom ) {
		rc.bottom++;
		}	
	}

void CGraphView::DrawSelection( _In_ CDC* pdc ) {
	ASSERT_VALID( pdc );
	auto Document = static_cast< CDirstatDoc* >( m_pDocument );;
	if ( Document != NULL ) {
		const auto item = Document->GetSelection( );
		if ( item == NULL ) {//no selection to draw.
			return;
			}
		CRect rcClient;
		GetClientRect( rcClient );

		auto rc = item->TmiGetRectangle( );

		TweakSizeOfRectangleForHightlight( rc, rcClient );

		CSelectStockObject sobrush( pdc, NULL_BRUSH );
		auto Options = GetOptions( );
		if ( Options != NULL ) {
			CPen pen( PS_SOLID, 1, Options->GetTreemapHighlightColor( ) );
			CSelectObject sopen( pdc, &pen );
			}
		ASSERT( Options != NULL );
		RenderHighlightRectangle( pdc, rc );
		}
	ASSERT( Document != NULL );
	}

void CGraphView::RenderHighlightRectangle( _In_ CDC* pdc, _In_ CRect& rc ) {
	/*
	  The documentation of CDC::Rectangle() says that the width and height must be greater than 2. Experiment says that it must be greater than 1. We follow the documentation.
	  A pen and the null brush must be selected.
	  */

	ASSERT_VALID( pdc );
	ASSERT( rc.Width( ) >= 0 );
	ASSERT( rc.Height( ) >= 0 );

	if ( rc.Width( ) >= 7 && rc.Height( ) >= 7 ) {
		pdc->Rectangle( rc );		// w = 7
		rc.DeflateRect( 1, 1 );
		pdc->Rectangle( rc );		// w = 5
		rc.DeflateRect( 1, 1 );
		pdc->Rectangle( rc );		// w = 3
		}
	else {
		auto Options = GetOptions( );
		ASSERT( Options != NULL );
		if ( Options != NULL ) {
			return pdc->FillSolidRect( rc, Options->GetTreemapHighlightColor( ) );
			}
		pdc->FillSolidRect( rc, RGB( 64, 64, 140 ) );//Fall back to some value
		}
	}

#ifdef _DEBUG
void CGraphView::AssertValid( ) const {
	CView::AssertValid( );
	}

void CGraphView::Dump( CDumpContext& dc ) const {
	CView::Dump( dc );
	}
#endif

_Must_inspect_result_ CDirstatDoc* CGraphView::GetDocument() {// Nicht-Debugversion ist inline
	ASSERT( m_pDocument->IsKindOf( RUNTIME_CLASS( CDirstatDoc ) ) );
	return static_cast< CDirstatDoc* >( m_pDocument );
	}
//#endif //_DEBUG



void CGraphView::OnSize( UINT nType, INT cx, INT cy ) {
	CView::OnSize( nType, cx, cy );
	CSize sz( cx, cy );
	if ( sz != m_size ) {
		Inactivate( );
		m_size = sz;
		}
	}

void CGraphView::OnLButtonDown( UINT nFlags, CPoint point ) {
	//auto Document = GetDocument( );
	auto Document = static_cast< CDirstatDoc* >( m_pDocument );
	if ( Document != NULL ) {
		auto root = Document->GetRootItem( );
		if ( root != NULL && root->IsDone( ) && IsDrawn( ) ) {
			auto item = static_cast<const CItemBranch*>( m_treemap.FindItemByPoint( Document->GetZoomItem( ), point ) );
			if ( item == NULL ) {
				goto noItemOrDocument;
				}

			Document->SetSelection( item );
			Document->UpdateAllViews( NULL, HINT_SHOWNEWSELECTION );
			}
		}
	else {
		TRACE( _T( "User clicked on nothing. User CAN click on nothing. That's a sane case.\r\n" ) );
		}
noItemOrDocument://Yeah, I hate it, but goto CAN be the cleanest solution in certain low-level cases.
	CView::OnLButtonDown( nFlags, point );
	}

bool CGraphView::IsDrawn( ) {
	return m_bitmap.m_hObject != NULL;
	}

void CGraphView::Inactivate( ) {
	//TODO: this function gets called waaay too much. Why are we REsetting every pixel to RGB( 100, 100, 100 ) on every update?? 
	if ( m_bitmap.m_hObject != NULL ) {
		// Move the old bitmap to m_dimmed
		m_dimmed.DeleteObject( );
		m_dimmed.Attach( m_bitmap.Detach( ) );
		m_dimmedSize = m_size;

		// Dimm m_inactive
		CClientDC dc( this );
		CDC dcmem;
		dcmem.CreateCompatibleDC( &dc );
		CSelectObject sobmp( &dcmem, &m_dimmed );
		for ( INT x = 0; x < m_dimmedSize.cx; x += 2 )
		for ( INT y = 0; y < m_dimmedSize.cy; y += 2 ) {
			ASSERT( ( x % 2 ) == 0 );
			ASSERT( ( y % 2 ) == 0 );
			dcmem.SetPixel( x, y, RGB( 100, 100, 100 ) );
			}
		}
	}

void CGraphView::EmptyView( ) {
	if ( m_bitmap.m_hObject != NULL ) {
		m_bitmap.DeleteObject( );
		}
	if ( m_dimmed.m_hObject != NULL ) {
		m_dimmed.DeleteObject( );
		}
	}

void CGraphView::OnSetFocus(CWnd* /*pOldWnd*/) {
	auto MainFrame = GetMainFrame( );
	if ( MainFrame != NULL ) {
		auto DirstatView = MainFrame->GetDirstatView( );
		if ( DirstatView != NULL ) {
			auto junk = DirstatView->SetFocus( );
			if ( junk != NULL ) {
				junk = NULL;//Don't use return CWnd* right now.
				}
			else if ( junk == NULL ) {
				TRACE( _T( "I'm told I set focus to NULL. That's weird.\r\n" ) );
				}
			}
		ASSERT( DirstatView != NULL );
		}
	ASSERT( MainFrame != NULL );
	}

void CGraphView::OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint ) {
	if ( !( static_cast< CDirstatDoc* >( m_pDocument ) )->IsRootDone( ) ) {
		Inactivate( );
		}

	switch ( lHint )
	{
		case HINT_NEWROOT:
			EmptyView( );
			return CView::OnUpdate( pSender, lHint, pHint );

		case HINT_SELECTIONCHANGED:
		case HINT_SHOWNEWSELECTION:
		case HINT_SELECTIONSTYLECHANGED:
		case HINT_EXTENSIONSELECTIONCHANGED:
			return CView::OnUpdate( pSender, lHint, pHint );

		case HINT_ZOOMCHANGED:
			Inactivate( );
			return CView::OnUpdate( pSender, lHint, pHint );

		case HINT_REDRAWWINDOW:
			RedrawWindow( );
			break;

		case HINT_TREEMAPSTYLECHANGED:
			Inactivate( );
			return CView::OnUpdate( pSender, lHint, pHint );

		case 0:
			return CView::OnUpdate( pSender, lHint, pHint );

		default:
			return;
	}
	}

void CGraphView::OnContextMenu(CWnd* /*pWnd*/, CPoint ptscreen) {
	auto Document = static_cast< CDirstatDoc* >( m_pDocument );
	if ( Document != NULL ) {
		auto root = Document->GetRootItem( );
		if ( root != NULL ) {
			if ( root->IsDone( ) ) {
				CMenu menu;
				menu.LoadMenu( IDR_POPUPGRAPH );
				auto sub = menu.GetSubMenu( 0 );
				if ( sub != NULL ) {
					sub->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, ptscreen.x, ptscreen.y, AfxGetMainWnd( ) );
					}
				ASSERT( sub != NULL );//How the fuck could we ever get NULL from that???!?
				}
			}
		else {
			TRACE( _T( "User tried to open a Context Menu, but there are no items in the Document. Well, they'll get what they asked for: a (NULL context) menu :)\r\n" ) );//(NULL context) menu == no context menu
			}
		}
	else { 
		TRACE( _T( "User tried to open a Context Menu, but the Document is NULL. Well, they'll get what they asked for: a (NULL context) menu :)\r\n" ) );//(NULL context) menu == no context menu
		}
	}

void CGraphView::OnMouseMove( UINT /*nFlags*/, CPoint point ) {
	auto Document = static_cast< CDirstatDoc* >( m_pDocument );
	if ( Document != NULL ) {
		auto root = Document->GetRootItem( );
		if ( root != NULL ) {
			if ( root->IsDone( ) && IsDrawn( ) ) {
				auto ZoomItem = Document->GetZoomItem( );
				if ( ZoomItem != NULL ) {
					auto item = ( const CItemBranch * ) m_treemap.FindItemByPoint( ZoomItem, point );
					if ( item != NULL ) {
						auto MainFrame = GetMainFrame( );
						if ( MainFrame != NULL ) {
							TRACE( _T( "Window is in focus, and Mouse is in the tree map area!( x: %ld, y: %ld ), Hovering over item: %s.\r\n" ), point.x, point.y, item->GetPath( ) );
							MainFrame->SetMessageText( ( item->GetPath( ) ) );
							}
						ASSERT( MainFrame != NULL );
						}
					else {
						TRACE( _T( "There's nothing with a path, therefore nothing for which we can set the message text.\r\n" ) );
						}
					}
				else {
					TRACE( _T( "FindItemByPoint CANNOT find a point when given a NULL ZoomItem! So let's not try.\r\n" ) );
					}
				}
			}
		}
		//Perhaps surprisingly, Document == NULL CAN be a valid condition. We don't have to set the message to anything if there's no document.
	if ( m_timer == 0 ) {
		TRACE( _T( "Mouse has left the tree map area?\r\n" ) );
		m_timer = SetTimer( 4711, 100, NULL );//TODO: figure out what the hell this does.//if value is increased ( argument 2 ), program execution will take longer to reach `TRACE( _T( "Mouse has left the tree map area!\r\n" ) );` after mouse has left tree map area.
		}
	}

void CGraphView::OnDestroy( ) {
	if ( m_timer != NULL ) {
		KillTimer( m_timer );
		}
	m_timer = 0;
	CView::OnDestroy( );
	}

void CGraphView::OnTimer( UINT_PTR /*nIDEvent*/ ) {
	CPoint point;
	GetCursorPos( &point );
	ScreenToClient( &point );

	CRect rc;
	GetClientRect( rc );

	if ( !rc.PtInRect( point ) ) {
		TRACE( _T( "Mouse has left the tree map area!\r\n" ) );
		GetMainFrame( )->SetSelectionMessageText( );
		KillTimer( m_timer );
		m_timer = 0;
		}
	}

void CGraphView::OnPopupCancel()
{
}

// $Log$
// Revision 1.6  2004/11/09 22:31:59  assarbad
// - Removed obsolete code lines
//
// Revision 1.5  2004/11/05 16:53:07  assarbad
// Added Date and History tag where appropriate.
//
