//////////////////////////////////////////////////////////////////////////////
// fluentexceptiondialog.cpp
// -------------------
//
// Copyright (c) 2009 Hugo Pereira Da Costa <hugo.pereira@free.fr>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//////////////////////////////////////////////////////////////////////////////

#include "fluentexceptiondialog.h"
#include "fluentdetectwidget.h"
#include "config-fluent.h"

#if FLUENT_HAVE_X11
#include <QX11Info>
#endif

namespace Fluent
{

    //___________________________________________
    ExceptionDialog::ExceptionDialog( QWidget* parent ):
        QDialog( parent )
    {

        m_ui.setupUi( this );

        connect( m_ui.buttonBox->button( QDialogButtonBox::Cancel ), SIGNAL(clicked()), this, SLOT(close()) );

        // detect window properties
        connect( m_ui.detectDialogButton, SIGNAL(clicked()), SLOT(selectWindowProperties()) );

        // connections
        connect( m_ui.exceptionType, SIGNAL(currentIndexChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.exceptionEditor, SIGNAL(textChanged(QString)), SLOT(updateChanged()) );

        for( CheckBoxMap::iterator iter = m_checkboxes.begin(); iter != m_checkboxes.end(); ++iter )
        { connect( iter.value(), SIGNAL(clicked()), SLOT(updateChanged()) ); }

        connect( m_ui.hideTitleBar, SIGNAL(clicked()), SLOT(updateChanged()) );
        connect( m_ui.opaqueTitleBar, SIGNAL(clicked()), SLOT(updateChanged()) );
        m_ui.opacityOverride->setSpecialValueText(tr("None"));
        connect( m_ui.opacityOverride, QOverload<int>::of(&QSpinBox::valueChanged), [=](int /*i*/){updateChanged();} );

        // hide detection dialog on non X11 platforms
        #if FLUENT_HAVE_X11
        if( !QX11Info::isPlatformX11() ) m_ui.detectDialogButton->hide();
        #else
        m_ui.detectDialogButton->hide();
        #endif
    }

    //___________________________________________
    void ExceptionDialog::setException( InternalSettingsPtr exception )
    {

        // store exception internally
        m_exception = exception;

        // type
        m_ui.exceptionType->setCurrentIndex(m_exception->exceptionType() );
        m_ui.exceptionEditor->setText( m_exception->exceptionPattern() );
        m_ui.hideTitleBar->setChecked( m_exception->hideTitleBar() );
        m_ui.opaqueTitleBar->setChecked( m_exception->opaqueTitleBar() );
        m_ui.opacityOverride->setValue( m_exception->opacityOverride() );

        // mask
        for( CheckBoxMap::iterator iter = m_checkboxes.begin(); iter != m_checkboxes.end(); ++iter )
        { iter.value()->setChecked( m_exception->mask() & iter.key() ); }

        setChanged( false );

    }

    //___________________________________________
    void ExceptionDialog::save()
    {
        m_exception->setExceptionType( m_ui.exceptionType->currentIndex() );
        m_exception->setExceptionPattern( m_ui.exceptionEditor->text() );
        m_exception->setHideTitleBar( m_ui.hideTitleBar->isChecked() );
        m_exception->setOpaqueTitleBar( m_ui.opaqueTitleBar->isChecked() );
        m_exception->setOpacityOverride( m_ui.opacityOverride->value() );

        // mask
        unsigned int mask = None;
        for( CheckBoxMap::iterator iter = m_checkboxes.begin(); iter != m_checkboxes.end(); ++iter )
        { if( iter.value()->isChecked() ) mask |= iter.key(); }

        m_exception->setMask( mask );

        setChanged( false );

    }

    //___________________________________________
    void ExceptionDialog::updateChanged()
    {
        bool modified( false );
        if( m_exception->exceptionType() != m_ui.exceptionType->currentIndex() ) modified = true;
        else if( m_exception->exceptionPattern() != m_ui.exceptionEditor->text() ) modified = true;
        else if( m_exception->hideTitleBar() != m_ui.hideTitleBar->isChecked() ) modified = true;
        else if( m_exception->opaqueTitleBar() != m_ui.opaqueTitleBar->isChecked() ) modified = true;
        else if( m_exception->opacityOverride() != m_ui.opacityOverride->value() ) modified = true;
        else
        {
            // check mask
            for( CheckBoxMap::iterator iter = m_checkboxes.begin(); iter != m_checkboxes.end(); ++iter )
            {
                if( iter.value()->isChecked() != (bool)( m_exception->mask() & iter.key() ) )
                {
                    modified = true;
                    break;
                }
            }
        }

        setChanged( modified );

    }

    //___________________________________________
    void ExceptionDialog::selectWindowProperties()
    {

        // create widget
        if( !m_detectDialog )
        {
            m_detectDialog = new DetectDialog( this );
            connect( m_detectDialog, SIGNAL(detectionDone(bool)), SLOT(readWindowProperties(bool)) );
        }

        m_detectDialog->detect(0);

    }

    //___________________________________________
    void ExceptionDialog::readWindowProperties( bool valid )
    {
        Q_CHECK_PTR( m_detectDialog );
        if( valid )
        {

            // type
            m_ui.exceptionType->setCurrentIndex( m_detectDialog->exceptionType() );

            // window info
            const KWindowInfo& info( m_detectDialog->windowInfo() );

            switch( m_detectDialog->exceptionType() )
            {

                default:
                case InternalSettings::ExceptionWindowClassName:
                m_ui.exceptionEditor->setText( QString::fromUtf8( info.windowClassClass() ) );
                break;

                case InternalSettings::ExceptionWindowTitle:
                m_ui.exceptionEditor->setText( info.name() );
                break;

            }

        }

        delete m_detectDialog;
        m_detectDialog = nullptr;

    }

}
