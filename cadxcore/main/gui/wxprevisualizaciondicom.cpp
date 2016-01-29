/*
*  
*  $Id: wxprevisualizaciondicom.cpp $
*  Ginkgo CADx Project
*
*  Copyright 2008-14 MetaEmotion S.L. All rights reserved.
*  http://ginkgo-cadx.com
*
*  This file is licensed under LGPL v3 license.
*  See License.txt for details
*
*
*/
#ifdef __DEPRECATED
#undef __DEPRECATED
#endif
#include "wxprevisualizaciondicom.h"

#include <vtkImageData.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <wxVTKRenderWindowInteractor.h>
#include <vtk/vtkginkgoimageviewer.h>
#include <vtk/interactor/ginkgointeractorstyleimage2d.h>


class wxPreviewVTKRenderWindowInteractor: public wxVTKRenderWindowInteractor
{
public:
	wxPreviewVTKRenderWindowInteractor(wxPrevisualizacionDicom *parent,
		wxWindowID id,
		const wxPoint &pos = wxDefaultPosition,
		const wxSize &size = wxDefaultSize,
		long style = wxWANTS_CHARS | wxNO_FULL_REPAINT_ON_RESIZE,
		const wxString &name = wxPanelNameStr) : wxVTKRenderWindowInteractor(parent, id, pos, size, style, name)
	{
		m_pParent = parent;
	}
	virtual void OnMouseDoubleClick(wxMouseEvent& event)
	{
		if (!Enabled || (ActiveButton != wxEVT_NULL))
		{
			return;
		}
		Activar();
		ActiveButton = event.GetEventType();

		// On Mac (Carbon) and Windows we don't automatically get the focus when
		// you click inside the window
		// we therefore set the focus explicitly
		// Apparently we need that on linux (GTK) too:
		this->SetFocus();

#if VTK_MAJOR_VERSION > 4 || (VTK_MAJOR_VERSION == 4 && VTK_MINOR_VERSION > 0)
		SetEventInformationFlipY(event.GetX(), event.GetY(),
			event.ControlDown(), event.ShiftDown(), '\0', 1, NULL);
#endif

		if (event.LeftDClick())
		{
			m_pParent->OnMouseLeftDClick();
		}

		ActiveButton = wxEVT_NULL;
		TransformarEvento(event);
	}
	wxPrevisualizacionDicom* m_pParent;
};

wxPrevisualizacionDicom::wxPrevisualizacionDicom(wxWindow* parent, wxWindowID ) : wxPanel(parent, wxID_ANY) {
	Sizer = new wxBoxSizer( wxHORIZONTAL );

	ViewInteractor = NULL;
	ViewInteractor = new wxPreviewVTKRenderWindowInteractor (this, wxID_ANY,
		wxDefaultPosition,
		wxDefaultSize,
		wxWANTS_CHARS | wxNO_FULL_REPAINT_ON_RESIZE,
		wxT(""));

	ViewImage2D = vtkGinkgoImageViewer::New();

	vtkRenderer* rend = vtkRenderer::New();

	ViewImage2D->SetRenderWindow ( this->ViewInteractor->GetRenderWindow() );
	ViewImage2D->SetRenderer (rend);

	ViewImage2D->SetBackgroundColor(0.0f,0.0f,0.0f);

	ViewImage2D->SetShowAnnotations(false);
	ViewImage2D->SetInteractionOn();
	GinkgoInteractorStyleImage2D* pInteractor = GinkgoInteractorStyleImage2D::SafeDownCast(ViewImage2D->GetInteractorStyle().GetPointer());
	if (pInteractor != NULL) {
		pInteractor->SetPreviewMode(true);
	}

	ViewImage2D->SetOrientation(vtkGinkgoImageViewer::AXIAL_ID);

	rend->Delete();

	this->SetSizer( Sizer );

	Sizer->Add( ViewInteractor, 1, wxEXPAND );


	this->Layout();
}

wxPrevisualizacionDicom::~wxPrevisualizacionDicom() {
	reset();
}

void wxPrevisualizacionDicom::SetInput(vtkSmartPointer<vtkImageData>& data) 
{
	if (data == NULL) {
		reset();
	} else {
		ViewImage2D->SetInput(data, true);
		ViewImage2D->ResetZoom(false, 0);
		ViewInteractor->Render();	
	}
}

void wxPrevisualizacionDicom::reset() {
	if (ViewImage2D != NULL) {
		ViewImage2D->Detach();
		ViewImage2D->Delete();
	}
	if (ViewInteractor != NULL) {
		ViewInteractor->Delete();
	}
}

void wxPrevisualizacionDicom::OnMouseLeftDClick() {}
