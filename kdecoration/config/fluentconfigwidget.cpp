//////////////////////////////////////////////////////////////////////////////
// fluentconfigurationui.cpp
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

#include "fluentconfigwidget.h"
#include "fluentexceptionlist.h"
#include "fluentsettings.h"

#include <KLocalizedString>

#include <QDBusConnection>
#include <QDBusMessage>

namespace Fluent
{

    //_________________________________________________________
    ConfigWidget::ConfigWidget( QWidget* parent, const QVariantList &args ):
        KCModule(parent, args),
        m_configuration( KSharedConfig::openConfig( QStringLiteral( "fluentrc" ) ) ),
        m_changed( false )
    {

        // configuration
        m_ui.setupUi( this );

        // track ui changes
        connect( m_ui.titleAlignment, SIGNAL(currentIndexChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.titleBarOpacity, QOverload<int>::of(&QSpinBox::valueChanged), [=](int /*i*/){updateChanged();} );

        // track animations changes
        connect( m_ui.animationsEnabled, SIGNAL(clicked()), SLOT(updateChanged()) );
        connect( m_ui.animationsDuration, SIGNAL(valueChanged(int)), SLOT(updateChanged()) );

        // track shadows changes
        connect( m_ui.shadowSize, SIGNAL(currentIndexChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.shadowStrength, SIGNAL(valueChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.shadowColor, SIGNAL(changed(QColor)), SLOT(updateChanged()) );

        // track exception changes
        connect( m_ui.exceptions, SIGNAL(changed(bool)), SLOT(updateChanged()) );

    }

    //_________________________________________________________
    void ConfigWidget::load()
    {

        // create internal settings and load from rc files
        m_internalSettings = InternalSettingsPtr( new InternalSettings() );
        m_internalSettings->load();

        // assign to ui
        m_ui.titleAlignment->setCurrentIndex( m_internalSettings->titleAlignment() );
        m_ui.animationsEnabled->setChecked( m_internalSettings->animationsEnabled() );
        m_ui.animationsDuration->setValue( m_internalSettings->animationsDuration() );
        m_ui.titleBarOpacity->setValue( m_internalSettings->backgroundOpacity() );

        // load shadows
        if( m_internalSettings->shadowSize() <= InternalSettings::ShadowVeryLarge ) m_ui.shadowSize->setCurrentIndex( m_internalSettings->shadowSize() );
        else m_ui.shadowSize->setCurrentIndex( InternalSettings::ShadowLarge );

        m_ui.shadowStrength->setValue( qRound(qreal(m_internalSettings->shadowStrength()*100)/255 ) );
        m_ui.shadowColor->setColor( m_internalSettings->shadowColor() );

        // load exceptions
        ExceptionList exceptions;
        exceptions.readConfig( m_configuration );
        m_ui.exceptions->setExceptions( exceptions.get() );
        setChanged( false );

    }

    //_________________________________________________________
    void ConfigWidget::save()
    {

        // create internal settings and load from rc files
        m_internalSettings = InternalSettingsPtr( new InternalSettings() );
        m_internalSettings->load();

        // apply modifications from ui
        m_internalSettings->setTitleAlignment( m_ui.titleAlignment->currentIndex() );
        m_internalSettings->setAnimationsEnabled( m_ui.animationsEnabled->isChecked() );
        m_internalSettings->setAnimationsDuration( m_ui.animationsDuration->value() );
        m_internalSettings->setBackgroundOpacity(m_ui.titleBarOpacity->value());

        m_internalSettings->setShadowSize( m_ui.shadowSize->currentIndex() );
        m_internalSettings->setShadowStrength( qRound( qreal(m_ui.shadowStrength->value()*255)/100 ) );
        m_internalSettings->setShadowColor( m_ui.shadowColor->color() );

        // save configuration
        m_internalSettings->save();

        // get list of exceptions and write
        InternalSettingsList exceptions( m_ui.exceptions->exceptions() );
        ExceptionList( exceptions ).writeConfig( m_configuration );

        // sync configuration
        m_configuration->sync();
        setChanged( false );

        // needed to tell kwin to reload when running from external kcmshell
        {
            QDBusMessage message = QDBusMessage::createSignal("/KWin", "org.kde.KWin", "reloadConfig");
            QDBusConnection::sessionBus().send(message);
        }

        // needed for fluent style to reload shadows
        {
            QDBusMessage message( QDBusMessage::createSignal("/FluentDecoration",  "org.kde.Fluent.Style", "reparseConfiguration") );
            QDBusConnection::sessionBus().send(message);
        }

    }

    //_________________________________________________________
    void ConfigWidget::defaults()
    {

        // create internal settings and load from rc files
        m_internalSettings = InternalSettingsPtr( new InternalSettings() );
        m_internalSettings->setDefaults();

        // assign to ui
        m_ui.titleAlignment->setCurrentIndex( m_internalSettings->titleAlignment() );
        m_ui.animationsEnabled->setChecked( m_internalSettings->animationsEnabled() );
        m_ui.animationsDuration->setValue( m_internalSettings->animationsDuration() );
        m_ui.titleBarOpacity->setValue( m_internalSettings->backgroundOpacity() );

        m_ui.shadowSize->setCurrentIndex( m_internalSettings->shadowSize() );
        m_ui.shadowStrength->setValue( qRound(qreal(m_internalSettings->shadowStrength()*100)/255 ) );
        m_ui.shadowColor->setColor( m_internalSettings->shadowColor() );

    }

    //_______________________________________________
    void ConfigWidget::updateChanged()
    {

        // check configuration
        if( !m_internalSettings ) return;

        // track modifications
        bool modified( false );

        if( m_ui.titleAlignment->currentIndex() != m_internalSettings->titleAlignment() ) modified = true;
        else if( m_ui.titleBarOpacity->value() != m_internalSettings->backgroundOpacity() ) modified = true;

        // animations
        else if( m_ui.animationsEnabled->isChecked() !=  m_internalSettings->animationsEnabled() ) modified = true;
        else if( m_ui.animationsDuration->value() != m_internalSettings->animationsDuration() ) modified = true;

        // shadows
        else if( m_ui.shadowSize->currentIndex() !=  m_internalSettings->shadowSize() ) modified = true;
        else if( qRound( qreal(m_ui.shadowStrength->value()*255)/100 ) != m_internalSettings->shadowStrength() ) modified = true;
        else if( m_ui.shadowColor->color() != m_internalSettings->shadowColor() ) modified = true;

        // exceptions
        else if( m_ui.exceptions->isChanged() ) modified = true;

        setChanged( modified );

    }

    //_______________________________________________
    void ConfigWidget::setChanged( bool value )
    {
        emit changed( value );
    }

}
