/*
 * This file is part of Ginkgo CADx
 *
 * Copyright (c) 2015-2016 Gert Wollny
 * Copyright (c) 2008-2014 MetaEmotion S.L. All rights reserved.
 *
 * Ginkgo CADx is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser Public License
 * along with Ginkgo CADx; if not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <api/internationalization/internationalization.h>
#include <api/controllers/ipacscontroller.h>
#include <api/controllers/ieventscontroller.h>
#include <main/controllers/commandcontroller.h>
#include <api/controllers/icontroladorlog.h>
#include <eventos/mensajes.h>
#include <main/entorno.h>
#include <main/controllers/historycontroller.h>
#include <eventos/progresocomando.h>
#include "acquirefromurlcommand.h"
#include "comandoincluirhistorial.h"
#include <wx/filename.h>
#include <wx/thread.h>


#define IDC_ACQUIRE_FROM_URL_COMMAND                  123

// Singleton de persistencia
namespace GADAPI
{
AcquireFromURLCommandParameters::AcquireFromURLCommandParameters(const std::list<std::string>& urlList) :
        m_urlList(urlList),
        m_pSemWait(new wxSemaphore())
{
}

AcquireFromURLCommandParameters::~AcquireFromURLCommandParameters()
{
        if (m_pSemWait != NULL) {
                delete m_pSemWait;
                m_pSemWait = NULL;
        }
}

AcquireFromURLCommand::AcquireFromURLCommand(AcquireFromURLCommandParameters* pParams) : IComando(pParams)
{
        m_pAcquireParams = pParams;
        SetId(IDC_ACQUIRE_FROM_URL_COMMAND);
        GNC::Entorno::Instance()->GetControladorEventos()->Registrar(this, GNC::GCS::Events::EventoProgresoComando());
}

void AcquireFromURLCommand::Execute()
{
        std::string tarea= _Std("Downloading files ...");
        if (!NotificarProgreso(0.0f,tarea)) {
                return;
        }

        GIL::DICOM::IPACSController* pCI = NULL;

        std::string pathDownloadedFiles = GNC::Entorno::Instance()->CrearDirectorioTemporal();
        try {
                pCI = GNC::GCS::IEntorno::Instance()->GetPACSController();
                int fileNum = 1;
                for (std::list<std::string>::const_iterator it = m_pAcquireParams->m_urlList.begin();
                     it != m_pAcquireParams->m_urlList.end(); ++it) {
                        {
                                if (!NotificarProgreso(fileNum/m_pAcquireParams->m_urlList.size(),
                                                       std::string(wxString::Format(_("Downloading file %d"), fileNum).ToUTF8()))) {
                                        return;
                                }
                        }
                        ++fileNum;
                        std::ostringstream ostr;
                        ostr << pathDownloadedFiles;
                        ostr << (char)wxFileName::GetPathSeparator();
                        ostr << fileNum;

                        if (!pCI->DownloadFileFromURL((*it), ostr.str(), this)) {
                                std::ostringstream ostrErr;
                                ostrErr << _Std("Error downloading file: ");
                                ostrErr << (*it);
                                throw GIL::DICOM::PACSException(ostrErr.str());
                        }
                }
        } catch (GIL::DICOM::PACSException& ex) {
                m_pAcquireParams->m_error =  (const std::string)ex;
        } catch (std::exception& ex) {
                m_pAcquireParams->m_error= ex.what();
        } catch (...) {
                m_pAcquireParams->m_error= _Std("Internal Error");
        }
        //including history....
        if(m_pAcquireParams->m_error.empty()) {
                GADAPI::ComandoIncluirHistorial::ComandoIncluirHistorialParams* pParams = new GADAPI::ComandoIncluirHistorial::ComandoIncluirHistorialParams(pathDownloadedFiles, true, GNC::GCS::IHistoryController::TAA_MOVE);
                pParams->m_abrirDespuesDeCargar = true;
                GADAPI::ComandoIncluirHistorial::ComandoIncluirHistorial * pCmd = new GADAPI::ComandoIncluirHistorial::ComandoIncluirHistorial(pParams);
                GNC::Entorno::Instance()->GetCommandController()->ProcessAsync("Processing...",pCmd,this);
                m_pAcquireParams->m_pSemWait->Wait();
        }

        NotificarProgreso(1.0f,tarea);
}

void AcquireFromURLCommand::Update()
{
        if (EstaAbortado()) {
                return;
        }
        if(!m_pAcquireParams->m_error.empty()) {
                GNC::GCS::IEventsController::Instance()->ProcesarEvento(new GNC::GCS::Events::EventoMensajes(NULL, _Std("Error downloading files: ") + "\n" + m_pAcquireParams->m_error, GNC::GCS::Events::EventoMensajes::PopUpMessage,false, GNC::GCS::Events::EventoMensajes::Error));
                return;
        }
}



bool AcquireFromURLCommand::NotificarProgreso(float progresoNormalizado,const std::string &texto)
{
        if (EstaAbortado()) {
                return false;
        }
        return IComando::NotificarProgreso(progresoNormalizado, texto);
}

void AcquireFromURLCommand::LiberarRecursos()
{
}

void AcquireFromURLCommand::ProcesarEvento(GNC::GCS::Events::IEvent *evt)
{
        GNC::GCS::Events::EventoProgresoComando* pEvt = dynamic_cast<GNC::GCS::Events::EventoProgresoComando*> (evt);
        if (!pEvt) {
                LOG_TRACE("Net", "AcquireFromURLCommand::ProcesarEvento: unexpected event type, ignore");
                return;
        }

        GNC::GCS::IComando* pCmd = pEvt->GetComando();
        if (pCmd != NULL && pCmd->GetOwner() == this &&pEvt->GetTipo() == GNC::GCS::Events::EventoProgresoComando::TEP_Finalizado) {
                m_pAcquireParams->m_pSemWait->Post();
        }
}

}
