// SpaceCollider -- SGI Lab Task 9
// Copyright (c) 2021-2022 Ángel Pérez <aperpor@upv.edu.es>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <Tunnel.hpp>

Tunnel::Tunnel()
{
}

void Tunnel::update(float dt, Player& player)
{
    Util::consolePrint("ENDD %f", (player.getPosition() - trajectory(m_endChunk)).magnitude());

    m_nearestChunk = chunkFromPosition(player.getPosition());
    m_distanceFromNearestChunk = (player.getPosition() - trajectory(m_nearestChunk)).magnitude();

    if (const auto distanceFromEnd = (player.getPosition() - trajectory(m_endChunk)).magnitude();
        distanceFromEnd < k_minGenDistance) {
        // Player is approaching the end of the tunnel
        m_endChunk++; // Render more chunks

        if (m_endChunk % k_energyCellPeriod == 0) {
            // Spawn a new energy cell
            m_energyCells.push_back(EnergyCell(trajectory(m_endChunk)));
            m_energyCells[m_energyCells.size() - 1].bindCollisionCallback([&player](const auto&) {
                player.rechargeFuel();
            });
        }

        if ((m_endChunk - m_startChunk) > k_maxChunks)
            m_startChunk++;
    }

    if (const auto distanceFromStart = (player.getPosition() - trajectory(m_startChunk)).magnitude();
        distanceFromStart < k_minGenDistance && (m_endChunk - m_startChunk) > k_minChunks) {
        // Player is approaching the start of the tunnel
        m_endChunk--; // Render more chunks
    }

    for (auto& energyCell : m_energyCells)
        energyCell.update(dt, player);

    m_playerPosition = player.getPosition();
}

void Tunnel::render() const
{
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render light threads for every chunk of the tunnel
    for (unsigned int thread = 0; thread < k_lightThreads; thread++) {
        float alpha = 0.75f * abs(sinf(thread * 64));
        const auto theta = Util::k_tau * (thread / static_cast<float>(k_lightThreads));
        const auto dx = k_tunnelRadius * cos(theta),
                   dy = k_tunnelRadius * sin(theta);

        glLineWidth(3.0f);
        glBegin(GL_LINE_STRIP);
        for (unsigned int chunk = m_startChunk; chunk <= m_endChunk; chunk++) {
            const auto position = trajectory(chunk);

            if (chunk >= m_nearestChunk + 50)
                alpha -= 0.005f;

            const float diffuseColor[] { 1, 0, 0, alpha };
            const float emissionColor[] { 1, 0, 0, 1 };

            glMaterialfv(GL_FRONT, GL_AMBIENT, diffuseColor);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseColor);
            glMaterialfv(GL_FRONT, GL_EMISSION, emissionColor);

            glVertex3f(dx + position.x, dy + position.y, position.z);
        }
        glEnd();
        glLineWidth(1.0f);
    }

    glDepthMask(GL_TRUE);
    glPopAttrib();

    renderBottomLight();

    for (const auto& energyCell : m_energyCells)
        energyCell.render();
}

void Tunnel::renderBottomLight() const
{
    const float lightAmbient[4] { 0, 0, 0, 0 };
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);

    const float lightPosition[4] { m_playerPosition.x, m_playerPosition.y - 2, m_playerPosition.z, 1 };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    const float lightColor[4] { 1, 0, 0, 1 };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);

    const float specularColor[4] { 1, 0, 0, 1 };
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularColor);

    const float spotDirection[3] { 0, 1, 0 };
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 90);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spotDirection);
    float x = pow(Util::k_e, m_distanceFromNearestChunk / (k_chunkLength));
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, x);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, x);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, x);

    glEnable(GL_LIGHT0);
}