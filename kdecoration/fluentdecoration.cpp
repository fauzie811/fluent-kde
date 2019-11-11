/*
* Copyright 2014  Martin Gräßlin <mgraesslin@kde.org>
* Copyright 2014  Hugo Pereira Da Costa <hugo.pereira@free.fr>
* Copyright 2018  Vlad Zagorodniy <vladzzag@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License or (at your option) version 3 or any later version
* accepted by the membership of KDE e.V. (or its successor approved
* by the membership of KDE e.V.), which shall act as a proxy
* defined in Section 14 of version 3 of the license.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "fluentdecoration.h"

#include "fluent.h"
#include "fluentsettingsprovider.h"
#include "config-fluent.h"
#include "config/fluentconfigwidget.h"

#include "fluentbutton.h"

#include "fluentboxshadowrenderer.h"

#include <KDecoration2/DecoratedClient>
#include <KDecoration2/DecorationButtonGroup>
#include <KDecoration2/DecorationSettings>
#include <KDecoration2/DecorationShadow>

#include <KConfigGroup>
#include <KColorUtils>
#include <KSharedConfig>
#include <KPluginFactory>

#include <QPainter>
#include <QTextStream>
#include <QTimer>

#if FLUENT_HAVE_X11
#include <QX11Info>
#endif

#include <cmath>

K_PLUGIN_FACTORY_WITH_JSON(
    FluentDecoFactory,
    "fluent.json",
    registerPlugin<Fluent::Decoration>();
    registerPlugin<Fluent::Button>(QStringLiteral("button"));
    registerPlugin<Fluent::ConfigWidget>(QStringLiteral("kcmodule"));
)

namespace
{
    struct ShadowParams {
        ShadowParams()
            : offset(QPoint(0, 0))
            , radius(0)
            , opacity(0) {}

        ShadowParams(const QPoint &offset, int radius, qreal opacity)
            : offset(offset)
            , radius(radius)
            , opacity(opacity) {}

        QPoint offset;
        int radius;
        qreal opacity;
    };

    struct CompositeShadowParams {
        CompositeShadowParams() = default;

        CompositeShadowParams(
                const QPoint &offset,
                const ShadowParams &shadow1,
                const ShadowParams &shadow2)
            : offset(offset)
            , shadow1(shadow1)
            , shadow2(shadow2) {}

        bool isNone() const {
            return qMax(shadow1.radius, shadow2.radius) == 0;
        }

        QPoint offset;
        ShadowParams shadow1;
        ShadowParams shadow2;
    };

    const CompositeShadowParams s_shadowParams[] = {
        // None
        CompositeShadowParams(),
        // Small
        CompositeShadowParams(
            QPoint(0, 4),
            ShadowParams(QPoint(0, 0), 16, 1),
            ShadowParams(QPoint(0, -2), 8, 0.4)),
        // Medium
        CompositeShadowParams(
            QPoint(0, 8),
            ShadowParams(QPoint(0, 0), 32, 0.9),
            ShadowParams(QPoint(0, -4), 16, 0.3)),
        // Large
        CompositeShadowParams(
            QPoint(0, 12),
            ShadowParams(QPoint(0, 0), 48, 0.8),
            ShadowParams(QPoint(0, -6), 24, 0.2)),
        // Very large
        CompositeShadowParams(
            QPoint(0, 16),
            ShadowParams(QPoint(0, 0), 64, 0.7),
            ShadowParams(QPoint(0, -8), 32, 0.1)),
    };

    inline CompositeShadowParams lookupShadowParams(int size)
    {
        switch (size) {
        case Fluent::InternalSettings::ShadowNone:
            return s_shadowParams[0];
        case Fluent::InternalSettings::ShadowSmall:
            return s_shadowParams[1];
        case Fluent::InternalSettings::ShadowMedium:
            return s_shadowParams[2];
        case Fluent::InternalSettings::ShadowLarge:
            return s_shadowParams[3];
        case Fluent::InternalSettings::ShadowVeryLarge:
            return s_shadowParams[4];
        default:
            // Fallback to the Large size.
            return s_shadowParams[3];
        }
    }
}

namespace Fluent
{

    using KDecoration2::ColorRole;
    using KDecoration2::ColorGroup;
    using KDecoration2::DecorationButtonType;

    //________________________________________________________________
    static int g_sDecoCount = 0;
    static int g_shadowSizeEnum = InternalSettings::ShadowLarge;
    static int g_shadowStrength = 255;
    static QColor g_shadowColor = Qt::black;
    static QSharedPointer<KDecoration2::DecorationShadow> g_sShadow;

    //________________________________________________________________
    Decoration::Decoration(QObject *parent, const QVariantList &args)
        : KDecoration2::Decoration(parent, args)
        , m_animation( new QPropertyAnimation( this ) )
    {
        g_sDecoCount++;
    }

    //________________________________________________________________
    Decoration::~Decoration()
    {
        g_sDecoCount--;
        if (g_sDecoCount == 0) {
            // last deco destroyed, clean up shadow
            g_sShadow.clear();
        }
    }

    //________________________________________________________________
    void Decoration::setOpacity( qreal value )
    {
        if( m_opacity == value ) return;
        m_opacity = value;
        update();
    }

    //________________________________________________________________
    QColor Decoration::titleBarColor() const
    {

        auto c = client().data();
        if( hideTitleBar() ) return c->color( ColorGroup::Inactive, ColorRole::TitleBar );
        else if( m_animation->state() == QPropertyAnimation::Running )
        {
            return KColorUtils::mix(
                c->color( ColorGroup::Inactive, ColorRole::TitleBar ),
                c->color( ColorGroup::Active, ColorRole::TitleBar ),
                m_opacity );
        } else return c->color( c->isActive() ? ColorGroup::Active : ColorGroup::Inactive, ColorRole::TitleBar );

    }

    //________________________________________________________________
    QColor Decoration::fontColor() const
    {

        auto c = client().data();
        if( m_animation->state() == QPropertyAnimation::Running )
        {
            return KColorUtils::mix(
                c->color( ColorGroup::Inactive, ColorRole::Foreground ),
                c->color( ColorGroup::Active, ColorRole::Foreground ),
                m_opacity );
        } else return  c->color( c->isActive() ? ColorGroup::Active : ColorGroup::Inactive, ColorRole::Foreground );

    }

    //________________________________________________________________
    void Decoration::init()
    {
        auto c = client().data();

        // active state change animation
        m_animation->setStartValue( 0 );
        m_animation->setEndValue( 1.0 );
        m_animation->setTargetObject( this );
        m_animation->setPropertyName( "opacity" );
        m_animation->setEasingCurve( QEasingCurve::InOutQuad );

        reconfigure();
        updateTitleBar();
        auto s = settings();

        // a change in font might cause the borders to change
        connect(s.data(), &KDecoration2::DecorationSettings::fontChanged, this, &Decoration::recalculateBorders);
        connect(s.data(), &KDecoration2::DecorationSettings::spacingChanged, this, &Decoration::recalculateBorders);

        // buttons
        connect(s.data(), &KDecoration2::DecorationSettings::spacingChanged, this, &Decoration::updateButtonsGeometryDelayed);
        connect(s.data(), &KDecoration2::DecorationSettings::decorationButtonsLeftChanged, this, &Decoration::updateButtonsGeometryDelayed);
        connect(s.data(), &KDecoration2::DecorationSettings::decorationButtonsRightChanged, this, &Decoration::updateButtonsGeometryDelayed);

        // full reconfiguration
        connect(s.data(), &KDecoration2::DecorationSettings::reconfigured, this, &Decoration::reconfigure);
        connect(s.data(), &KDecoration2::DecorationSettings::reconfigured, SettingsProvider::self(), &SettingsProvider::reconfigure, Qt::UniqueConnection );
        connect(s.data(), &KDecoration2::DecorationSettings::reconfigured, this, &Decoration::updateButtonsGeometryDelayed);

        connect(c, &KDecoration2::DecoratedClient::adjacentScreenEdgesChanged, this, &Decoration::recalculateBorders);
        connect(c, &KDecoration2::DecoratedClient::maximizedHorizontallyChanged, this, &Decoration::recalculateBorders);
        connect(c, &KDecoration2::DecoratedClient::maximizedVerticallyChanged, this, &Decoration::recalculateBorders);
        connect(c, &KDecoration2::DecoratedClient::shadedChanged, this, &Decoration::recalculateBorders);
        connect(c, &KDecoration2::DecoratedClient::captionChanged, this,
            [this]()
            {
                // update the caption area
                update(titleBar());
            }
        );

        connect(c, &KDecoration2::DecoratedClient::activeChanged, this, &Decoration::updateAnimationState);
        connect(c, &KDecoration2::DecoratedClient::widthChanged, this, &Decoration::updateTitleBar);
        connect(c, &KDecoration2::DecoratedClient::maximizedChanged, this, &Decoration::updateTitleBar);
        // connect(c, &KDecoration2::DecoratedClient::maximizedChanged, this, &Decoration::setOpaque);

        connect(c, &KDecoration2::DecoratedClient::widthChanged, this, &Decoration::updateButtonsGeometry);
        connect(c, &KDecoration2::DecoratedClient::maximizedChanged, this, &Decoration::updateButtonsGeometry);
        connect(c, &KDecoration2::DecoratedClient::adjacentScreenEdgesChanged, this, &Decoration::updateButtonsGeometry);
        connect(c, &KDecoration2::DecoratedClient::shadedChanged, this, &Decoration::updateButtonsGeometry);

        createButtons();
        createShadow();
    }

    //________________________________________________________________
    void Decoration::updateTitleBar()
    {
        auto c = client().data();
        setTitleBar(QRect(0, 0, c->width(), borderTop()));
    }

    //________________________________________________________________
    void Decoration::updateAnimationState()
    {
        if( m_internalSettings->animationsEnabled() )
        {

            auto c = client().data();
            m_animation->setDirection( c->isActive() ? QPropertyAnimation::Forward : QPropertyAnimation::Backward );
            if( m_animation->state() != QPropertyAnimation::Running ) m_animation->start();

        } else {

            update();

        }
    }

    //________________________________________________________________
    void Decoration::reconfigure()
    {

        m_internalSettings = SettingsProvider::self()->internalSettings( this );

        // animation
        m_animation->setDuration( m_internalSettings->animationsDuration() );

        // borders
        recalculateBorders();

        // shadow
        createShadow();
    }

    //________________________________________________________________
    void Decoration::recalculateBorders()
    {
        auto s = settings();

        int top = 0;
        if( !hideTitleBar() ) {
            QFontMetrics fm(s->font());
            top += qMax(fm.height(), buttonHeight() );
        }

        setBorders(QMargins(0, top, 0, 0));

        // extended sizes
        const int extSize = s->largeSpacing();
        setResizeOnlyBorders(QMargins(extSize, extSize, extSize, extSize));
    }

    //________________________________________________________________
    void Decoration::createButtons()
    {
        m_leftButtons = new KDecoration2::DecorationButtonGroup(KDecoration2::DecorationButtonGroup::Position::Left, this, &Button::create);
        m_rightButtons = new KDecoration2::DecorationButtonGroup(KDecoration2::DecorationButtonGroup::Position::Right, this, &Button::create);
        updateButtonsGeometry();
    }

    //________________________________________________________________
    void Decoration::updateButtonsGeometryDelayed()
    { QTimer::singleShot( 0, this, &Decoration::updateButtonsGeometry ); }

    //________________________________________________________________
    void Decoration::updateButtonsGeometry()
    {
        const auto s = settings();

        // adjust button position
        const int bHeight = buttonHeight();
        foreach( const QPointer<KDecoration2::DecorationButton>& button, m_leftButtons->buttons() + m_rightButtons->buttons() )
        {
            const int bWidth = buttonHeight() * (button.data()->type() == DecorationButtonType::Menu ? 1.0 : 1.5);
            button.data()->setGeometry( QRectF( QPoint( 0, 0 ), QSizeF( bWidth, bHeight ) ) );
            static_cast<Button*>( button.data() )->setIconSize( QSize( bWidth, bHeight ) );
        }

        // left buttons
        if( !m_leftButtons->buttons().isEmpty() )
        {

            m_leftButtons->setSpacing(0);

            m_leftButtons->setPos(QPointF(0, 0));

        }

        // right buttons
        if( !m_rightButtons->buttons().isEmpty() )
        {

            m_rightButtons->setSpacing(0);

            // padding
            m_rightButtons->setPos(QPointF(size().width() - m_rightButtons->geometry().width(), 0));

        }

        update();

    }

    //________________________________________________________________
    void Decoration::paint(QPainter *painter, const QRect &repaintRegion)
    {
        // TODO: optimize based on repaintRegion
        auto s = settings();

        if( !hideTitleBar() ) paintTitleBar(painter, repaintRegion);

    }

    //________________________________________________________________
    void Decoration::paintTitleBar(QPainter *painter, const QRect &repaintRegion)
    {
        const auto c = client().data();
        const QRect titleRect(QPoint(0, 0), QSize(size().width(), buttonHeight()));

        if ( !titleRect.intersects(repaintRegion) ) return;

        painter->save();
        painter->setPen(Qt::NoPen);

        QColor titleBarColor = this->titleBarColor();
        titleBarColor.setAlpha(titleBarAlpha());

        painter->setBrush( titleBarColor );

        auto s = settings();
        if( isMaximized() || !s->isAlphaChannelSupported() )
        {

            painter->drawRect(titleRect);

        } else if( c->isShaded() ) {

            painter->drawRoundedRect(titleRect, Metrics::Frame_FrameRadius, Metrics::Frame_FrameRadius);

        } else {

            painter->setClipRect(titleRect, Qt::IntersectClip);

            // the rect is made a little bit larger to be able to clip away the rounded corners at the bottom and sides
            painter->drawRoundedRect(titleRect.adjusted(
                isLeftEdge() ? -Metrics::Frame_FrameRadius:0,
                isTopEdge() ? -Metrics::Frame_FrameRadius:0,
                isRightEdge() ? Metrics::Frame_FrameRadius:0,
                Metrics::Frame_FrameRadius),
                Metrics::Frame_FrameRadius, Metrics::Frame_FrameRadius);

        }

        painter->restore();

        // draw caption
        painter->setFont(s->font());
        painter->setPen( fontColor() );
        const auto cR = captionRect();
        const QString caption = painter->fontMetrics().elidedText(c->caption(), Qt::ElideMiddle, cR.first.width());
        painter->drawText(cR.first, cR.second | Qt::TextSingleLine, caption);

        // draw all buttons
        m_leftButtons->paint(painter, repaintRegion);
        m_rightButtons->paint(painter, repaintRegion);
    }

    //________________________________________________________________
    int Decoration::buttonHeight() const
    {
        return 30;
    }

    //________________________________________________________________
    int Decoration::buttonWidth() const
    {
        return 46;
    }

    //________________________________________________________________
    int Decoration::iconSize() const
    {
        return 16;
    }

    //________________________________________________________________
    QPair<QRect,Qt::Alignment> Decoration::captionRect() const
    {
        if( hideTitleBar() ) return qMakePair( QRect(), Qt::AlignCenter );
        else {

            auto c = client().data();
            int leftOffset = m_leftButtons->buttons().isEmpty() ?
                4.0*settings()->smallSpacing():
                m_leftButtons->geometry().x() + m_leftButtons->geometry().width() + 4.0*settings()->smallSpacing();

            if (!m_leftButtons->buttons().isEmpty() 
                && m_leftButtons->buttons().last().data()->type() == DecorationButtonType::Menu 
                && m_internalSettings->titleAlignment() == InternalSettings::AlignLeft)
            {
                leftOffset -= 4.0 * settings()->smallSpacing();
            }
            
            const int rightOffset = m_rightButtons->buttons().isEmpty() ?
                4.0*settings()->smallSpacing() :
                size().width() - m_rightButtons->geometry().x() + 4.0*settings()->smallSpacing();

            const QRect maxRect( leftOffset, 0, size().width() - leftOffset - rightOffset, buttonHeight() );

            switch( m_internalSettings->titleAlignment() )
            {
                case InternalSettings::AlignLeft:
                return qMakePair( maxRect, Qt::AlignVCenter|Qt::AlignLeft );

                case InternalSettings::AlignRight:
                return qMakePair( maxRect, Qt::AlignVCenter|Qt::AlignRight );

                case InternalSettings::AlignCenter:
                return qMakePair( maxRect, Qt::AlignCenter );

                default:
                case InternalSettings::AlignCenterFullWidth:
                {

                    // full caption rect
                    const QRect fullRect = QRect( 0, 0, size().width(), buttonHeight() );
                    QRect boundingRect( settings()->fontMetrics().boundingRect( c->caption()).toRect() );

                    // text bounding rect
                    boundingRect.setTop( 0 );
                    boundingRect.setHeight( buttonHeight() );
                    boundingRect.moveLeft( ( size().width() - boundingRect.width() )/2 );

                    if( boundingRect.left() < leftOffset ) return qMakePair( maxRect, Qt::AlignVCenter|Qt::AlignLeft );
                    else if( boundingRect.right() > size().width() - rightOffset ) return qMakePair( maxRect, Qt::AlignVCenter|Qt::AlignRight );
                    else return qMakePair(fullRect, Qt::AlignCenter);

                }

            }

        }

    }

    //________________________________________________________________
    void Decoration::createShadow()
    {
        if (!g_sShadow
                ||g_shadowSizeEnum != m_internalSettings->shadowSize()
                || g_shadowStrength != m_internalSettings->shadowStrength()
                || g_shadowColor != m_internalSettings->shadowColor())
        {
            g_shadowSizeEnum = m_internalSettings->shadowSize();
            g_shadowStrength = m_internalSettings->shadowStrength();
            g_shadowColor = m_internalSettings->shadowColor();

            const CompositeShadowParams params = lookupShadowParams(g_shadowSizeEnum);
            if (params.isNone()) {
                g_sShadow.clear();
                setShadow(g_sShadow);
                return;
            }

            auto withOpacity = [](const QColor &color, qreal opacity) -> QColor {
                QColor c(color);
                c.setAlphaF(opacity);
                return c;
            };

            const QSize boxSize = BoxShadowRenderer::calculateMinimumBoxSize(params.shadow1.radius)
                .expandedTo(BoxShadowRenderer::calculateMinimumBoxSize(params.shadow2.radius));

            BoxShadowRenderer shadowRenderer;
            shadowRenderer.setBorderRadius(Metrics::Frame_FrameRadius + 0.5);
            shadowRenderer.setBoxSize(boxSize);
            shadowRenderer.setDevicePixelRatio(1.0); // TODO: Create HiDPI shadows?

            const qreal strength = static_cast<qreal>(g_shadowStrength) / 255.0;
            shadowRenderer.addShadow(params.shadow1.offset, params.shadow1.radius,
                withOpacity(g_shadowColor, params.shadow1.opacity * strength));
            shadowRenderer.addShadow(params.shadow2.offset, params.shadow2.radius,
                withOpacity(g_shadowColor, params.shadow2.opacity * strength));

            QImage shadowTexture = shadowRenderer.render();

            QPainter painter(&shadowTexture);
            painter.setRenderHint(QPainter::Antialiasing);

            const QRect outerRect = shadowTexture.rect();

            QRect boxRect(QPoint(0, 0), boxSize);
            boxRect.moveCenter(outerRect.center());

            // Mask out inner rect.
            const QMargins padding = QMargins(
                boxRect.left() - outerRect.left() - Metrics::Shadow_Overlap - params.offset.x(),
                boxRect.top() - outerRect.top() - Metrics::Shadow_Overlap - params.offset.y(),
                outerRect.right() - boxRect.right() - Metrics::Shadow_Overlap + params.offset.x(),
                outerRect.bottom() - boxRect.bottom() - Metrics::Shadow_Overlap + params.offset.y());
            const QRect innerRect = outerRect - padding;

            painter.setPen(Qt::NoPen);
            painter.setBrush(Qt::black);
            painter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
            painter.drawRoundedRect(
                innerRect,
                Metrics::Frame_FrameRadius + 0.5,
                Metrics::Frame_FrameRadius + 0.5);

            // Draw outline.
            painter.setPen(withOpacity(g_shadowColor, 0.2 * strength));
            painter.setBrush(Qt::NoBrush);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            painter.drawRoundedRect(
                innerRect,
                Metrics::Frame_FrameRadius - 0.5,
                Metrics::Frame_FrameRadius - 0.5);

            painter.end();

            g_sShadow = QSharedPointer<KDecoration2::DecorationShadow>::create();
            g_sShadow->setPadding(padding);
            g_sShadow->setInnerShadowRect(QRect(outerRect.center(), QSize(1, 1)));
            g_sShadow->setShadow(shadowTexture);
        }

        setShadow(g_sShadow);
    }
} // namespace


#include "fluentdecoration.moc"
