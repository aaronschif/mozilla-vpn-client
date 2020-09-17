#include "timercontroller.h"

#include <QDebug>

TimerController::TimerController(ControllerImpl *impl) : m_impl(impl)
{
    Q_ASSERT(m_impl);
    m_impl->setParent(this);

    connect(m_impl, &ControllerImpl::initialized, this, &ControllerImpl::initialized);
    connect(m_impl,
            &ControllerImpl::connected,
            [this] { TimerController::maybeDone(true); });
    connect(m_impl,
            &ControllerImpl::disconnected,
            [this] { TimerController::maybeDone(false); });

    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &TimerController::timeout);
}

void TimerController::initialize(const Device *device, const Keys *keys)
{
    m_impl->initialize(device, keys);
}

void TimerController::activate(const Server &server,
                               const Device *device,
                               const Keys *keys,
                               bool forSwitching)
{
    Q_ASSERT(m_state == None);
    m_state = Connecting;

    if (!forSwitching) {
        m_timer.stop();
        m_timer.start(TIME_ACTIVATION);
    }

    m_impl->activate(server, device, keys, forSwitching);
}

void TimerController::deactivate(const Server &server,
                                 const Device *device,
                                 const Keys *keys,
                                 bool forSwitching)
{
    Q_ASSERT(m_state == None);
    m_state = Disconnecting;

    m_timer.stop();
    m_timer.start(forSwitching ? TIME_SWITCHING : TIME_DEACTIVATION);

    m_impl->deactivate(server, device, keys, forSwitching);
}

void TimerController::timeout()
{
    qDebug() << "TimerController - Timeout:" << m_state;

    Q_ASSERT(m_state != None);

    if (m_state == Connected) {
        m_state = None;
        emit connected();
        return;
    }

    if (m_state == Disconnected) {
        m_state = None;
        emit disconnected();
        return;
    }

    // Any other state can be ignored.
}

void TimerController::maybeDone(bool isConnected)
{
    qDebug() << "TimerController - Operation completed:" << m_state << isConnected;

    if (m_state == Connecting) {
        if (m_timer.isActive()) {
            // The connection was faster.
            m_state = isConnected ? Connected : Disconnected;
            return;
        }

        // The timer was faster.
        m_state = None;

        if (isConnected) {
            emit connected();
        } else {
            emit disconnected();
        }
        return;
    }

    if (m_state == Disconnecting) {
        if (m_timer.isActive()) {
            // The disconnection was faster.
            m_state = Disconnected;
            return;
        }

        // The timer was faster.
        m_state = None;
        emit disconnected();
        return;
    }

    // External events could trigger the following codes.

    Q_ASSERT(m_state == None);
    Q_ASSERT(!m_timer.isActive());

    if (isConnected) {
        emit connected();
        return;
    }

    emit disconnected();
}
